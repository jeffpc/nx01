/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <nomad/objstore_backend.h>

#include "mem.h"

static int mem_vdev_getroot(struct objstore *vol, struct noid *root)
{
	struct memstore *ms = vol->vdev->private;

	mxlock(&ms->lock);
	*root = ms->root->oid;
	mxunlock(&ms->lock);

	return 0;
}

static int mem_allocobj(struct obj *obj)
{
	struct memstore *ms = obj->vol->vdev->private;
	struct memobj key = {
		.oid = obj->oid,
	};
	struct memobj *mobj;

	mxlock(&ms->lock);
	mobj = memobj_getref(avl_find(&ms->objs, &key, NULL));
	mxunlock(&ms->lock);

	if (!mobj)
		return -ENOENT;

	obj->nversions = avl_numnodes(&mobj->versions);
	obj->nlink = mobj->nlink;
	obj->private = mobj;
	obj->ops = &obj_ops;

	return 0;
}

/*
 * We're keeping this here to keep it close to mem_allocobj().  As the name
 * implies, this is an object op - not a vdev op.
 */
void mem_obj_free(struct obj *obj)
{
	struct memobj *mobj = obj->private;

	memobj_putref(mobj);
}

static const struct vol_ops vol_ops = {
	.getroot = mem_vdev_getroot,
	.allocobj = mem_allocobj,
};

static int objcmp(const void *va, const void *vb)
{
	const struct memobj *a = va;
	const struct memobj *b = vb;

	return noid_cmp(&a->oid, &b->oid);
}

static int mem_create(struct objstore_vdev *vdev)
{
	struct memstore *ms;
	struct memobj *obj;

	ms = malloc(sizeof(struct memstore));
	if (!ms)
		return -ENOMEM;

	ms->vdev = vdev;

	atomic_set(&ms->next_oid_uniq, 1);
	avl_create(&ms->objs, objcmp, sizeof(struct memobj),
		   offsetof(struct memobj, node));

	mxinit(&ms->lock);

	obj = newmemobj(ms, NATTR_DIR | 0777);
	if (IS_ERR(obj)) {
		free(ms);
		return PTR_ERR(obj);
	}

	obj->nlink++;

	avl_add(&ms->objs, memobj_getref(obj));
	ms->root = obj; /* hand off our reference */

	vdev->private = ms;

	return 0;
}

const struct objstore_vdev_def objvdev = {
	.name = "mem",

	.create = mem_create,
};
