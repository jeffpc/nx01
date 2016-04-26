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
#include <nomad/objstore_backend.h>

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

static struct memdentry *newdentry(struct memobj *child, const char *name)
{
	struct memdentry *dentry;
	char *dupname;

	if (!child || !name)
		return ERR_PTR(-EINVAL);

	dentry = malloc(sizeof(struct memdentry));
	if (!dentry)
		return ERR_PTR(-ENOMEM);

	dupname = strdup(name);
	if (!dupname)
		goto err;

	dentry->name = dupname;
	dentry->obj = memobj_getref(child);

	return dentry;

err:
	free(dentry);
	return ERR_PTR(-ENOMEM);
}

static struct memver *newobjver(uint16_t mode)
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
	ver->attrs.nlink = 0xBAAAAAAD; /* don't use this, use memobj's nlink */
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

struct memobj *newmemobj(struct memstore *ms, uint16_t mode)
{
	struct memver *ver;
	struct memobj *obj;
	int ret;

	ret = -ENOMEM;

	obj = malloc(sizeof(struct memobj));
	if (!obj)
		goto err;

	ver = newobjver(mode);
	if (IS_ERR(ver)) {
		ret = PTR_ERR(ver);
		goto err;
	}

	obj->nlink = 0;

	refcnt_init(&obj->refcnt, 1);

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

static void freedentry(struct memdentry *dentry)
{
	if (!dentry)
		return;

	memobj_putref(dentry->obj);
	free((void *) dentry->name);
	free(dentry);
}

static void freeobjver(struct memver *ver)
{
	if (!ver)
		return;

	avl_destroy(&ver->dentries);
	nvclock_free(ver->clock);
	free(ver);
}

void freememobj(struct memobj *obj)
{
	struct memver *ver;
	void *cookie;

	if (!obj)
		return;

	cookie = NULL;
	while ((ver = avl_destroy_nodes(&obj->versions, &cookie)))
		freeobjver(ver);

	avl_destroy(&obj->versions);
	free(obj);
}

struct memobj *findmemobj(struct memstore *store, const struct noid *oid)
{
	struct memobj key = {
		.oid = *oid,
	};

	return memobj_getref(avl_find(&store->objs, &key, NULL));
}

/*
 * returns a specific version of an object, with the object held
 */
struct memver *findver_by_hndl(struct memstore *store,
			       const struct noid *oid,
			       const struct nvclock *clock)
{
	struct memobj *obj;
	struct memver *ver;

	mxlock(&store->lock);
	obj = findmemobj(store, oid);
	mxunlock(&store->lock);

	if (!obj)
		return ERR_PTR(-ENOENT);

	switch (avl_numnodes(&obj->versions)) {
		case 0:
			break;
		case 1:
			ver = avl_first(&obj->versions);

			if (nvclock_is_null(clock) ||
			    nvclock_cmp(clock, ver->clock) == NVC_EQ)
				return ver;

			break;
		default: {
			struct memver key = {
				.clock = (struct nvclock *) clock,
			};

			if (nvclock_is_null(clock)) {
				memobj_putref(obj);
				return ERR_PTR(-ENOTUNIQ);
			}

			ver = avl_find(&obj->versions, &key, NULL);
			if (ver)
				return ver;

			break;
		}
	}

	memobj_putref(obj);

	return ERR_PTR(-ENOENT);
}

/*
 * We don't do I/O, instead we keep everything in memory.  This however
 * means that instead of doing I/O (which would write generic object and
 * object version state to disk) we need to copy the generic state into our
 * private backend structures.
 */
static void sync_ver_to_mver(struct objver *ver)
{
	struct memobj *mobj = ver->obj->private;
	struct memver *mver = ver->private;
	struct obj *obj = ver->obj;

	/* object */
	mobj->nlink = obj->nlink;

	/* version */
	nvclock_copy(mver->clock, ver->clock);
	mver->attrs = ver->attrs;
}

static int mem_obj_getversion(struct objver *ver)
{
	struct memstore *ms = ver->obj->vol->private;
	struct memver *mver;

	mver = findver_by_hndl(ms, &ver->obj->oid, ver->clock);
	if (IS_ERR(mver))
		return PTR_ERR(mver);

	ver->private = mver;

	ver->attrs = mver->attrs;

	return 0;
}

static void *mem_obj_open(struct objstore_vol *vol, const struct noid *oid,
			  const struct nvclock *clock)
{
	struct memstore *ms;
	struct memver *ver;

	if (!vol || !oid || !clock)
		return ERR_PTR(-EINVAL);

	ms = vol->private;

	ver = findver_by_hndl(ms, oid, clock);
	if (IS_ERR(ver))
		return ver;

	/*
	 * The object associated with 'ver' is held.  We keep the hold on
	 * it.  Then we return the pointer to the memver to the caller.
	 * Then, later on when the caller calls the close function, we
	 * release the object's hold.
	 *
	 * While the object version is open, we maintain this reference hold
	 * keeping the object and the version structures in memory.  Even if
	 * all links are removed, we keep the structures around and let
	 * operations use them.  This mimics the POSIX file system semantics
	 * well.
	 */

	return ver;
}

static int mem_obj_close(struct objstore_vol *vol, void *cookie)
{
	struct memver *ver = cookie;

	if (!vol || !ver)
		return -EINVAL;

	/*
	 * Yes, we are dereferencing the pointer that the caller supplied.
	 * Yes, this is safe.  The only reason this is safe is because we
	 * rely on the caller to keep the pointer safe.  Specifically, we
	 * assume that the caller didn't transmit the pointer over the
	 * network.
	 *
	 * See comment in mem_obj_open() for why we only putref the object.
	 */

	memobj_putref(ver->obj);

	return 0;
}

static int mem_obj_getattr(struct objver *ver, struct nattr *attr)
{
	*attr = ver->attrs;
	attr->nlink = ver->obj->nlink;

	return 0;
}

/*
 * Truncate the blob buffer.
 *
 * The objver's size is updated to the new size, and the memver's blob is
 * reallocated as necessary.  If zero is true, any newly allocated bytes are
 * zeroed.
 */
static int __truncate(struct objver *ver, uint64_t newsize, bool zero)
{
	struct memver *mver = ver->private;
	void *tmp;

	if (newsize == ver->attrs.size)
		return 0;

	if (newsize < ver->attrs.size) {
		ver->attrs.size = newsize;
		return 0;
	}

	tmp = realloc(mver->blob, newsize);
	if (!tmp)
		return -ENOMEM;

	/* clear the new bytes */
	if (zero)
		memset(tmp + ver->attrs.size, 0,
		       newsize - ver->attrs.size);

	ver->attrs.size = newsize;

	mver->blob = tmp;

	return 0;
}

static int mem_obj_setattr(struct objver *ver, const struct nattr *attr,
			   const unsigned valid)
{
	int ret;

	/*
	 * first do some checks
	 */
	if ((valid & OBJ_ATTR_SIZE) && !NATTR_ISREG(ver->attrs.mode))
		return -EINVAL;

	/* we can't change the type of the object */
	if ((valid & OBJ_ATTR_MODE) &&
	    (attr->mode & NATTR_TMASK) != (ver->attrs.mode & NATTR_TMASK))
		return -EINVAL;

	/*
	 * now do the updates
	 */
	/* must be first since it can fail */
	if (valid & OBJ_ATTR_SIZE) {
		ret = __truncate(ver, attr->size, true);
		if (ret)
			return ret;
	}

	if (valid & OBJ_ATTR_MODE)
		ver->attrs.mode = attr->mode;

	/* TODO: do we need to tweak the versions AVL tree? */
	nvclock_inc(ver->clock);

	sync_ver_to_mver(ver);

	return 0;
}

static ssize_t mem_obj_read(struct objver *ver, void *buf, size_t len,
			    uint64_t offset)
{
	struct memver *mver = ver->private;
	ssize_t ret;

	if (offset >= ver->attrs.size)
		ret = 0;
	else if ((offset + len) > ver->attrs.size)
		ret = ver->attrs.size - offset;
	else
		ret = len;

	if (ret)
		memcpy(buf, mver->blob + offset, ret);

	return ret;
}

static ssize_t mem_obj_write(struct objver *ver, const void *buf, size_t len,
			     uint64_t offset)
{
	struct memver *mver = ver->private;
	ssize_t ret;

	/* object will grow - need to resize the blob buffer */
	if ((offset + len) > ver->attrs.size) {
		ret = __truncate(ver, offset + len, false);
		if (ret)
			return ret;
	}

	memcpy(mver->blob + offset, buf, len);

	 /* TODO: do we need to tweak the versions AVL tree? */
	nvclock_inc(ver->clock);

	sync_ver_to_mver(ver);

	return len;
}

static int mem_obj_lookup(struct objver *dirver, const char *name,
			  struct noid *child)
{
	const struct memdentry key = {
		.name = name,
	};
	struct memver *dirmver = dirver->private;
	struct memdentry *dentry;

	dentry = avl_find(&dirmver->dentries, &key, NULL);
	if (!dentry)
		return -ENOENT;

	/*
	 * In theory, we should lock the child, but we don't have to
	 * because:
	 *
	 * (1) the child can't go away - the dentry is holding a reference
	 *     and we have the directory itself locked
	 * (2) the oid never changes
	 */

	*child = dentry->obj->oid;

	return 0;
}

/* returns a referenced child object */
static struct memobj *__obj_create(struct memstore *store, struct memver *dir,
				   const char *name, uint16_t mode)
{
	struct memdentry *dentry;
	struct memobj *mchild;

	/* allocate the child object */
	mchild = newmemobj(store, mode);
	if (IS_ERR(mchild))
		return mchild;

	/* allocate the dentry */
	dentry = newdentry(mchild, name);
	if (IS_ERR(dentry)) {
		memobj_putref(mchild);
		return ERR_CAST(dentry);
	}

	/* add the dentry to the parent */
	avl_add(&dir->dentries, dentry);

	mchild->nlink++;

	/* add object to the global list */
	mxlock(&store->lock);
	avl_add(&store->objs, memobj_getref(mchild));
	mxunlock(&store->lock);

	return mchild;
}

static int mem_obj_create(struct objver *dirver, const char *name,
			  uint16_t mode, struct noid *child)
{
	const struct memdentry key = {
		.name = name,
	};
	struct memstore *ms = dirver->obj->vol->private;
	struct memver *dirmver = dirver->private;
	struct memobj *childobj;

	if (avl_find(&dirmver->dentries, &key, NULL))
		return -EEXIST;

	childobj = __obj_create(ms, dirmver, name, mode);
	if (IS_ERR(childobj))
		return PTR_ERR(childobj);

	/* set return oid */
	*child = childobj->oid;

	memobj_putref(childobj);

	/*
	 * We changed the dir, so we need to up the version.
	 *
	 * TODO: do we need to tweak the dentries AVL tree?
	 */
	nvclock_inc(dirver->clock);

	sync_ver_to_mver(dirver);

	return 0;
}

static int __obj_unlink(struct memstore *store, struct memver *dir,
			struct memdentry *dentry)
{
	struct memobj *child;

	/* get a reference for us */
	child = memobj_getref(dentry->obj);

	/* remove the dentry from the directory */
	avl_remove(&dir->dentries, dentry);

	/* free the dentry */
	freedentry(dentry);

	/* if there are no more links to the child obj, free it */
	if (!--child->nlink) {
		mxlock(&store->lock);
		avl_remove(&store->objs, child);
		mxunlock(&store->lock);

		/* put the global list's ref */
		memobj_putref(child);
	}

	/* put our ref */
	memobj_putref(child);

	/*
	 * We changed the dir, so we need to up the version.
	 *
	 * TODO: do we need to tweak the versions AVL tree?
	 */
	nvclock_inc(dir->clock);

	return 0;
}

static int mem_obj_unlink(struct objstore_vol *vol, void *dircookie,
			  const char *name)
{
	const struct memdentry key = {
		.name = name,
	};
	struct memdentry *dentry;
	struct memver *dirver = dircookie;
	struct memstore *ms;

	if (!vol || !dirver || !name)
		return -EINVAL;

	ms = vol->private;

	dentry = avl_find(&dirver->dentries, &key, NULL);
	if (!dentry)
		return -ENOENT;

	/* ok, we got the dentry - remove it */
	return __obj_unlink(ms, dirver, dentry);
}

const struct obj_ops obj_ops = {
	.getversion = mem_obj_getversion,
	.open    = mem_obj_open,
	.close   = mem_obj_close,
	.getattr = mem_obj_getattr,
	.setattr = mem_obj_setattr,
	.read    = mem_obj_read,
	.write   = mem_obj_write,
	.lookup  = mem_obj_lookup,
	.create  = mem_obj_create,
	.unlink  = mem_obj_unlink,
	.free    = mem_obj_free,
};
