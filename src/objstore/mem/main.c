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
#include <sys/list.h>
#include <stdlib.h>
#include <stddef.h>

#include <nomad/error.h>
#include <nomad/time.h>
#include <nomad/atomic.h>
#include <nomad/objstore_impl.h>

/* each version of an an object */
struct memobjver {
	struct nvclock *version;
	struct nattr attrs;
	void *blob;
	list_node_t node;
};

/* each object */
struct memobj {
	struct noid oid;
	avl_node_t node;
	list_t versions;
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
	return noid_cmp(va, vb);
}

static struct memobj *newobj(uint16_t mode)
{
	struct memobj *obj;
	struct memobjver *ver;
	int ret;

	ret = ENOMEM;

	obj = malloc(sizeof(struct memobj));
	ver = malloc(sizeof(struct memobjver));
	if (!obj || !ver)
		goto err;

	list_create(&obj->versions, sizeof(struct memobjver),
		    offsetof(struct memobjver, node));

	ver->version = nvclock_alloc();
	if (!ver->version)
		goto err;

	ret = nvclock_set(ver->version, 1);
	if (ret)
		goto err_vector;

	ver->attrs._reserved = 0;
	ver->attrs.mode = mode;
	ver->attrs.nlink = 0;
	ver->attrs.size = 0;
	ver->attrs.atime = gettime();
	ver->attrs.btime = ver->attrs.atime;
	ver->attrs.ctime = ver->attrs.atime;
	ver->attrs.mtime = ver->attrs.atime;

	list_insert_tail(&obj->versions, ver);

	return obj;

err_vector:
	nvclock_free(ver->version);

err:
	free(ver);
	free(obj);

	return ERR_PTR(ret);
}

static int mem_store_create(struct objstore *store)
{
	struct memstore *ms;

	ms = malloc(sizeof(struct memstore));
	if (!ms)
		return ENOMEM;

	atomic_set(&ms->next_oid_uniq, 1);
	avl_create(&ms->objs, cmp, sizeof(struct memobj),
		   offsetof(struct memobj, node));

	ms->root = newobj(NATTR_DIR | 0777);
	if (IS_ERR(ms->root)) {
		int ret = PTR_ERR(ms->root);

		free(ms);
		return ret;
	}

	ms->ds = 0x1234; /* TODO: this should be generated randomly */

	noid_set(&ms->root->oid, ms->ds, atomic_inc(&ms->next_oid_uniq));

	avl_add(&ms->objs, ms->root);

	store->private = ms;

	return 0;
}

static const struct objstore_ops store_ops = {
	.create = mem_store_create,
};

static const struct obj_ops obj_ops = {
};

const struct objstore_def objstore = {
	.name = "mem",
	.store_ops = &store_ops,
	.obj_ops = &obj_ops,
};
