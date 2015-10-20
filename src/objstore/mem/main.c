/*
 * Copyright (c) 2015 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <sys/avl.h>
#include <stdlib.h>
#include <stddef.h>

#include <nomad/error.h>
#include <nomad/time.h>
#include <nomad/rand.h>
#include <nomad/atomic.h>
#include <nomad/mutex.h>
#include <nomad/objstore_impl.h>

/* each <oid,ver> */
struct memobj {
	/* key */
	struct nobjhndl handle;

	/* value */
	struct nattr attrs;
	void *blob; /* used if the memobj is a file */

	avl_tree_t dentries; /* used if the memobj is a director */

	/* misc */
	avl_node_t node;
};

struct mem_dentry {
	const char *name;
	struct nobjhndl *handle;
	avl_node_t node;
};

static int dentry_cmp(const void *a, const void *b)
{
	const struct mem_dentry *na = a;
	const struct mem_dentry *nb = b;
	int ret;

	ret = strcmp(na->name, nb->name);
	if (ret > 0)
		return 1;
	else if (ret < 0)
		return -1;
	return 0;
}

/* the whole store */
struct memstore {
	avl_tree_t objs;
	struct memobj *root;

	uint32_t ds; /* our dataset id */
	atomic64_t next_oid_uniq; /* the next unique part of noid */

	pthread_mutex_t lock;
};

static int cmp(const void *va, const void *vb)
{
	const struct memobj *a = va;
	const struct memobj *b = vb;
	int ret;

	ret = noid_cmp(&a->handle.oid, &b->handle.oid);
	if (ret)
		return ret;

	return nvclock_cmp_total(a->handle.clock, b->handle.clock);
}

static struct memobj *newobj(uint16_t mode)
{
	struct memobj *obj;
	int ret;

	ret = ENOMEM;

	obj = malloc(sizeof(struct memobj));
	if (!obj)
		goto err;

	obj->handle.clock = nvclock_alloc();
	if (!obj->handle.clock)
		goto err;

	ret = nvclock_set(obj->handle.clock, 1);
	if (ret)
		goto err_vector;

	if (NATTR_ISDIR(mode))
		avl_create(&obj->dentries, dentry_cmp, sizeof(struct mem_dentry),
		           offsetof(struct mem_dentry, node));

	obj->blob = NULL;
	obj->attrs._reserved = 0;
	obj->attrs.mode = mode;
	obj->attrs.nlink = 0;
	obj->attrs.size = 0;
	obj->attrs.atime = gettime();
	obj->attrs.btime = obj->attrs.atime;
	obj->attrs.ctime = obj->attrs.atime;
	obj->attrs.mtime = obj->attrs.atime;

	return obj;

err_vector:
	nvclock_free(obj->handle.clock);

err:
	free(obj);

	return ERR_PTR(ret);
}

static int mem_vol_create(struct objstore_vol *store)
{
	struct memstore *ms;
	struct memobj *obj;

	ms = malloc(sizeof(struct memstore));
	if (!ms)
		return ENOMEM;

	atomic_set(&ms->next_oid_uniq, 1);
	avl_create(&ms->objs, cmp, sizeof(struct memobj),
		   offsetof(struct memobj, node));

	mxinit(&ms->lock);

	ms->ds = rand32();

	obj = newobj(NATTR_DIR | 0777);
	if (IS_ERR(obj)) {
		free(ms);
		return PTR_ERR(obj);
	}

	obj->attrs.nlink = 1;
	noid_set(&obj->handle.oid, ms->ds, atomic_inc(&ms->next_oid_uniq));

	ms->root = obj;
	avl_add(&ms->objs, obj);

	store->private = ms;

	return 0;
}

static int mem_vol_getroot(struct objstore_vol *store, struct nobjhndl *hndl)
{
	struct memstore *ms;
	int ret;

	if (!store || !store->private || !hndl)
		return EINVAL;

	ms = store->private;

	mxlock(&ms->lock);
	ret = nobjhndl_cpy(hndl, &ms->root->handle);
	mxunlock(&ms->lock);

	return ret;
}

static struct memobj * __mem_obj_lookup(struct memstore *store,
                                        const struct nobjhndl *hndl)
{
	struct memobj key;

	key.handle.oid = hndl->oid;
	key.handle.clock = hndl->clock;

	return avl_find(&store->objs, &key, NULL);
}

static int mem_obj_getattr(struct objstore_vol *vol, const struct nobjhndl *hndl,
			   struct nattr *attr)
{
	struct memstore *ms;
	struct memobj *obj;
	int ret;

	if (!vol || !hndl || !attr)
		return EINVAL;

	ms = vol->private;

	mxlock(&ms->lock);
	obj = __mem_obj_lookup(ms, hndl);
	mxunlock(&ms->lock);

	if (!obj) {
		ret = ENOENT;
	} else {
		ret = 0;
		*attr = obj->attrs;
	}

	return ret;
}

static int mem_obj_lookup(struct objstore_vol *vol, const struct nobjhndl *dir,
                          const char *name, struct nobjhndl *child)
{
	const struct mem_dentry key = {
		.name = name,
	};
	struct memstore *ms;
	struct mem_dentry *dentry;
	struct memobj *dirobj;

	if (!vol || !dir || !name || !child)
		return EINVAL;

	ms = vol->private;

	mxlock(&ms->lock);
	dirobj = __mem_obj_lookup(ms, dir);
	mxunlock(&ms->lock);

	if (!dirobj)
		return ENOENT;

	/* These objects will eventually need their own locks, too. */
	dentry = avl_find(&dirobj->dentries, &key, NULL);
	if (!dentry)
		return ENOENT;

	return nobjhndl_cpy(child, dentry->handle);
}

static const struct vol_ops vol_ops = {
	.create = mem_vol_create,
	.getroot = mem_vol_getroot,
};

static const struct obj_ops obj_ops = {
	.getattr = mem_obj_getattr,
	.lookup  = mem_obj_lookup,
};

const struct objstore_vol_def objvol = {
	.name = "mem",
	.vol_ops = &vol_ops,
	.obj_ops = &obj_ops,
};
