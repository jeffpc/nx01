/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/error.h>

#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

struct mem_cache *obj_cache;
struct mem_cache *objver_cache;

static int ver_cmp(const void *va, const void *vb)
{
	const struct objver *a = va;
	const struct objver *b = vb;

	return nvclock_cmp_total(a->clock, b->clock);
}

/*
 * Allocate a new generic object structure.
 */
struct obj *allocobj(void)
{
	struct obj *obj;

	obj = mem_cache_alloc(obj_cache);
	if (!obj)
		return NULL;

	memset(&obj->oid, 0, sizeof(obj->oid));

	avl_create(&obj->versions, ver_cmp, sizeof(struct objver),
		   offsetof(struct objver, node));

	obj->nversions = 0;
	obj->nlink = 0;
	obj->private = NULL;
	obj->state = OBJ_STATE_NEW;
	obj->vol = NULL;
	obj->ops = NULL;

	refcnt_init(&obj->refcnt, 1);
	mxinit(&obj->lock);

	return obj;
}

/*
 * Free a generic object structure.
 */
void freeobj(struct obj *obj)
{
	if (!obj)
		return;

	if (obj->ops && obj->ops->free)
		obj->ops->free(obj);

	mxdestroy(&obj->lock);
	vol_putref(obj->vol);
	avl_destroy(&obj->versions);
	mem_cache_free(obj_cache, obj);
}

/*
 * Allocate a new generic object version structure.
 */
struct objver *allocobjver(void)
{
	struct objver *ver;
	int ret;

	ver = mem_cache_alloc(objver_cache);
	if (!ver)
		return ERR_PTR(-ENOMEM);

	ver->clock = nvclock_alloc(false);
	if (!ver->clock) {
		ret = -ENOMEM;
		goto err;
	}

	memset(&ver->attrs, 0, sizeof(ver->attrs));

	ver->private = NULL;
	ver->open_count = 0;
	ver->obj = NULL;

	return ver;

err:
	mem_cache_free(objver_cache, ver);

	return ERR_PTR(ret);
}

/*
 * Free a generic object version structure.
 */
void freeobjver(struct objver *ver)
{
	if (!ver)
		return;

	nvclock_free(ver->clock);
	mem_cache_free(objver_cache, ver);
}
