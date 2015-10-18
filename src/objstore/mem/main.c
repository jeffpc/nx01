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

#include <sys/avl.h>
#include <stdlib.h>
#include <stddef.h>

#include <nomad/error.h>
#include <nomad/time.h>
#include <nomad/rand.h>
#include <nomad/atomic.h>
#include <nomad/objstore_impl.h>

/* each <oid,ver> */
struct memobj {
	/* key */
	struct noid oid;
	struct nvclock *ver;

	/* value */
	struct nattr attrs;
	void *blob;

	/* misc */
	avl_node_t node;
};

/* the whole store */
struct memstore {
	avl_tree_t objs;
	struct memobj *root;

	uint32_t ds; /* our dataset id */
	atomic64_t next_oid_uniq; /* the next unique part of noid */
};

static int cmp(const void *va, const void *vb)
{
	const struct memobj *a = va;
	const struct memobj *b = vb;
	int ret;

	ret = noid_cmp(&a->oid, &b->oid);
	if (ret)
		return ret;

	return nvclock_cmp_total(a->ver, b->ver);
}

static struct memobj *newobj(uint16_t mode)
{
	struct memobj *obj;
	int ret;

	ret = ENOMEM;

	obj = malloc(sizeof(struct memobj));
	if (!obj)
		goto err;

	obj->ver = nvclock_alloc();
	if (!obj->ver)
		goto err;

	ret = nvclock_set(obj->ver, 1);
	if (ret)
		goto err_vector;

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
	nvclock_free(obj->ver);

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

	obj = newobj(NATTR_DIR | 0777);
	if (IS_ERR(obj)) {
		free(ms);
		return PTR_ERR(obj);
	}

	ms->ds = rand32();

	noid_set(&obj->oid, ms->ds, atomic_inc(&ms->next_oid_uniq));

	ms->root = obj;
	avl_add(&ms->objs, obj);

	store->private = ms;

	return 0;
}

static int mem_vol_getroot(struct objstore_vol *store, struct nobjhndl *hndl)
{
	struct memstore *ms;

	if (!store || !store->private || !hndl)
		return EINVAL;

	ms = store->private;

	hndl->oid = ms->root->oid;
	hndl->clock = nvclock_dup(ms->root->ver);

	if (!hndl->clock)
		return ENOMEM;

	return 0;
}

static const struct vol_ops vol_ops = {
	.create = mem_vol_create,
	.getroot = mem_vol_getroot,
};

static const struct obj_ops obj_ops = {
};

const struct objstore_vol_def objvol = {
	.name = "mem",
	.vol_ops = &vol_ops,
	.obj_ops = &obj_ops,
};
