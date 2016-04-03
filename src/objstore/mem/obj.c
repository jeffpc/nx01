/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * Copyright (c) 2015 Holly Sipek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stddef.h>
#include <umem.h>

#include <jeffpc/error.h>

#include <nomad/time.h>
#include <nomad/objstore_impl.h>

#include "mem.h"

static int ver_cmp(const void *va, const void *vb)
{
	const struct memver *a = va;
	const struct memver *b = vb;

	return nvclock_cmp_total(a->clock, b->clock);
}

static int dentry_cmp(const void *a, const void *b)
{
	const struct memdentry *na = a;
	const struct memdentry *nb = b;
	int ret;

	ret = strcmp(na->name, nb->name);
	if (ret > 0)
		return 1;
	else if (ret < 0)
		return -1;
	return 0;
}

static struct memver *newobjver(uint16_t mode, uint32_t nlink)
{
	struct memver *ver;
	int ret;

	ver = malloc(sizeof(struct memver));
	if (!ver) {
		ret = -ENOMEM;
		goto err;
	}

	ver->clock = nvclock_alloc();
	if (!ver->clock) {
		ret = -ENOMEM;
		goto err_free;
	}

	ret = nvclock_set(ver->clock, 1);
	if (ret)
		goto err_free;

	avl_create(&ver->dentries, dentry_cmp, sizeof(struct memdentry),
	           offsetof(struct memdentry, node));

	ver->blob = NULL;
	ver->attrs._reserved = 0;
	ver->attrs.mode = mode;
	ver->attrs.nlink = nlink;
	ver->attrs.size = 0;
	ver->attrs.atime = gettime();
	ver->attrs.btime = ver->attrs.atime;
	ver->attrs.ctime = ver->attrs.atime;
	ver->attrs.mtime = ver->attrs.atime;

	return ver;

err_free:
	free(ver);

err:
	return ERR_PTR(ret);
}

struct memobj *newobj(struct memstore *ms, uint16_t mode, uint32_t nlink)
{
	struct memver *ver;
	struct memobj *obj;
	int ret;

	ret = -ENOMEM;

	obj = malloc(sizeof(struct memobj));
	if (!obj)
		goto err;

	ver = newobjver(mode, nlink);
	if (IS_ERR(ver)) {
		ret = PTR_ERR(ver);
		goto err;
	}

	atomic_set(&obj->refcnt, 1);
	mxinit(&obj->lock);

	avl_create(&obj->versions, ver_cmp, sizeof(struct memver),
		   offsetof(struct memver, node));

	noid_set(&obj->oid, ms->ds, atomic_inc(&ms->next_oid_uniq));

	avl_add(&obj->versions, ver);
	ver->obj = obj;

	return obj;

err:
	free(obj);

	return ERR_PTR(ret);
}

static void freeobjver(struct memver *ver)
{
	if (!ver)
		return;

	avl_destroy(&ver->dentries);
	nvclock_free(ver->clock);
	free(ver);
}

void freeobj(struct memobj *obj)
{
	struct memver *ver;
	void *cookie;

	if (!obj)
		return;

	cookie = NULL;
	while ((ver = avl_destroy_nodes(&obj->versions, &cookie)))
		freeobjver(ver);

	avl_destroy(&obj->versions);
	mxdestroy(&obj->lock);
	free(obj);
}

struct memobj *findobj(struct memstore *store, const struct noid *oid)
{
	struct memobj key = {
		.oid = *oid,
	};

	return memobj_getref(avl_find(&store->objs, &key, NULL));
}

/*
 * returns a specific version of an object, with the object held & locked
 */
struct memver *findver_by_hndl(struct memstore *store,
			       const struct nobjhndl *hndl)
{
	struct memobj *obj;
	struct memver *ver;

	mxlock(&store->lock);
	obj = findobj(store, &hndl->oid);
	mxunlock(&store->lock);

	if (!obj)
		return ERR_PTR(-ENOENT);

	mxlock(&obj->lock);
	switch (avl_numnodes(&obj->versions)) {
		case 0:
			break;
		case 1:
			ver = avl_first(&obj->versions);

			if (nvclock_is_null(hndl->clock) ||
			    nvclock_cmp(hndl->clock, ver->clock) == NVC_EQ)
				return ver;

			break;
		default: {
			struct memver key = {
				.clock = hndl->clock,
			};

			if (nvclock_is_null(hndl->clock)) {
				mxunlock(&obj->lock);
				memobj_putref(obj);
				return ERR_PTR(-ENOTUNIQ);
			}

			ver = avl_find(&obj->versions, &key, NULL);
			if (ver)
				return ver;

			break;
		}
	}

	mxunlock(&obj->lock);

	memobj_putref(obj);

	return ERR_PTR(-ENOENT);
}

static int mem_obj_getattr(struct objstore_vol *vol, const struct nobjhndl *hndl,
			   struct nattr *attr)
{
	struct memstore *ms;
	struct memver *ver;

	if (!vol || !hndl || !attr)
		return -EINVAL;

	ms = vol->private;

	ver = findver_by_hndl(ms, hndl);
	if (IS_ERR(ver))
		return PTR_ERR(ver);

	*attr = ver->attrs;
	mxunlock(&ver->obj->lock);
	memobj_putref(ver->obj);

	return 0;
}

static int mem_obj_lookup(struct objstore_vol *vol, const struct nobjhndl *dir,
                          const char *name, struct noid *child)
{
	const struct memdentry key = {
		.name = name,
	};
	struct memstore *ms;
	struct memdentry *dentry;
	struct memver *dirver;
	int ret;

	if (!vol || !dir || !name || !child)
		return -EINVAL;

	ms = vol->private;

	dirver = findver_by_hndl(ms, dir);
	if (IS_ERR(dirver))
		return PTR_ERR(dirver);

	if (!NATTR_ISDIR(dirver->attrs.mode)) {
		ret = -ENOTDIR;
		goto err_unlock;
	}

	dentry = avl_find(&dirver->dentries, &key, NULL);
	if (!dentry) {
		ret = -ENOENT;
		goto err_unlock;
	}

	mxlock(&dentry->obj->lock);
	*child = dentry->obj->oid;
	mxunlock(&dentry->obj->lock);

	ret = 0;

err_unlock:
	mxunlock(&dirver->obj->lock);
	memobj_putref(dirver->obj);

	return ret;
}

/* returns a locked & referenced child object */
static struct memobj *__obj_create(struct memstore *store, struct memver *dir,
				   const char *name, uint16_t mode)
{
	struct memdentry *dentry;
	struct memobj *obj;
	int ret;

	/* allocate the child object */
	obj = newobj(store, mode, 1);
	if (IS_ERR(obj))
		return obj;

	/* allocate the dentry */
	dentry = malloc(sizeof(struct memdentry));
	if (!dentry) {
		ret = -ENOMEM;
		goto err;
	}

	/* dup the name for the dentry */
	dentry->name = strdup(name);
	if (!dentry->name) {
		ret = -ENOMEM;
		free(dentry);
		goto err;
	}

	/* lock the object */
	mxlock(&obj->lock);

	/* prepare the dentry */
	dentry->obj = memobj_getref(obj);

	/* add the dentry to the parent */
	avl_add(&dir->dentries, dentry);

	/*
	 * We changed the dir, so we need to up the version.
	 *
	 * TODO: do we need to tweak the dentries AVL tree?
	 */
	nvclock_inc(dir->clock);

	/* add object to the global list */
	mxlock(&store->lock);
	avl_add(&store->objs, memobj_getref(obj));
	mxunlock(&store->lock);

	return obj;

err:
	memobj_putref(obj);

	return ERR_PTR(ret);
}

static int mem_obj_create(struct objstore_vol *vol, const struct nobjhndl *dir,
			  const char *name, uint16_t mode,
			  struct noid *child)
{
	const struct memdentry key = {
		.name = name,
	};
	struct memstore *ms;
	struct memobj *childobj;
	struct memver *dirver;
	int ret;

	if (!vol || !dir || !name || !child)
		return -EINVAL;

	ms = vol->private;

	dirver = findver_by_hndl(ms, dir);
	if (IS_ERR(dirver))
		return PTR_ERR(dirver);

	if (!NATTR_ISDIR(dirver->attrs.mode)) {
		ret = -ENOTDIR;
		goto err_unlock;
	}

	if (avl_find(&dirver->dentries, &key, NULL)) {
		ret = -EEXIST;
		goto err_unlock;
	}

	childobj = __obj_create(ms, dirver, name, mode);
	if (IS_ERR(childobj)) {
		ret = PTR_ERR(childobj);
		goto err_unlock;
	}

	/* set return oid */
	*child = childobj->oid;
	ret = 0;

	mxunlock(&childobj->lock);
	memobj_putref(childobj);

err_unlock:
	mxunlock(&dirver->obj->lock);
	memobj_putref(dirver->obj);

	return ret;
}

const struct obj_ops obj_ops = {
	.getattr = mem_obj_getattr,
	.lookup  = mem_obj_lookup,
	.create  = mem_obj_create,
};
