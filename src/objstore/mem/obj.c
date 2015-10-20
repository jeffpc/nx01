/*
 * Copyright (c) 2015 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <nomad/error.h>
#include <nomad/time.h>
#include <nomad/objstore_impl.h>

#include "mem.h"

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

struct memobj *newobj(uint16_t mode)
{
	struct memobj *obj;
	int ret;

	ret = ENOMEM;

	obj = malloc(sizeof(struct memobj));
	if (!obj)
		goto err;

	atomic_set(&obj->refcnt, 1);

	obj->handle.clock = nvclock_alloc();
	if (!obj->handle.clock)
		goto err;

	ret = nvclock_set(obj->handle.clock, 1);
	if (ret)
		goto err_vector;

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

void freeobj(struct memobj *obj)
{
	if (!obj)
		return;

	avl_destroy(&obj->dentries);
	nvclock_free(obj->handle.clock);
	free(obj);
}

struct memobj *findobj(struct memstore *store, const struct nobjhndl *hndl)
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
	obj = findobj(ms, hndl);
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
	dirobj = findobj(ms, dir);
	mxunlock(&ms->lock);

	if (!dirobj)
		return ENOENT;

	if (!NATTR_ISDIR(dirobj->attrs.mode))
		return ENOTDIR;

	/* These objects will eventually need their own locks, too. */
	dentry = avl_find(&dirobj->dentries, &key, NULL);
	if (!dentry)
		return ENOENT;

	return nobjhndl_cpy(child, dentry->handle);
}

const struct obj_ops obj_ops = {
	.getattr = mem_obj_getattr,
	.lookup  = mem_obj_lookup,
};
