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

#include <stdlib.h>
#include <stddef.h>

#include <jeffpc/error.h>
#include <jeffpc/rand.h>

#include <nomad/objstore_impl.h>

#include "mem.h"

static int objcmp(const void *va, const void *vb)
{
	const struct memobj *a = va;
	const struct memobj *b = vb;

	return noid_cmp(&a->oid, &b->oid);
}

static int mem_vol_create(struct objstore_vol *store)
{
	struct memstore *ms;
	struct memobj *obj;

	ms = malloc(sizeof(struct memstore));
	if (!ms)
		return -ENOMEM;

	atomic_set(&ms->next_oid_uniq, 1);
	avl_create(&ms->objs, objcmp, sizeof(struct memobj),
		   offsetof(struct memobj, node));

	mxinit(&ms->lock);

	ms->ds = rand32();

	obj = newobj(ms, NATTR_DIR | 0777);
	if (IS_ERR(obj)) {
		free(ms);
		return PTR_ERR(obj);
	}

	obj->nlink++;

	avl_add(&ms->objs, memobj_getref(obj));
	ms->root = obj; /* hand off our reference */

	store->private = ms;

	return 0;
}

static int mem_vol_getroot(struct objstore_vol *store, struct noid *root)
{
	struct memstore *ms;

	if (!store || !store->private || !root)
		return -EINVAL;

	ms = store->private;

	mxlock(&ms->lock);
	*root = ms->root->oid;
	mxunlock(&ms->lock);

	return 0;
}

static const struct vol_ops vol_ops = {
	.create = mem_vol_create,
	.getroot = mem_vol_getroot,
};

const struct objstore_vol_def objvol = {
	.name = "mem",
	.vol_ops = &vol_ops,
	.obj_ops = &obj_ops,
};
