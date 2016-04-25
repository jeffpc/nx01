/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

umem_cache_t *obj_cache;
umem_cache_t *objver_cache;

int vol_getattr(struct objstore_vol *vol, void *cookie, struct nattr *attr)
{
	if (!vol || !attr)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->getattr)
		return -ENOTSUP;

	return vol->def->obj_ops->getattr(vol, cookie, attr);
}

int vol_setattr(struct objstore_vol *vol, void *cookie,
		const struct nattr *attr, const unsigned valid)
{
	if (!vol || !attr)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->setattr)
		return -ENOTSUP;

	return vol->def->obj_ops->setattr(vol, cookie, attr, valid);
}

ssize_t vol_read(struct objstore_vol *vol, void *cookie, void *buf, size_t len,
		 uint64_t offset)
{
	if (!vol || !buf)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->read)
		return -ENOTSUP;

	return vol->def->obj_ops->read(vol, cookie, buf, len, offset);
}

ssize_t vol_write(struct objstore_vol *vol, void *cookie, const void *buf,
		  size_t len, uint64_t offset)
{
	if (!vol || !buf)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->write)
		return -ENOTSUP;

	return vol->def->obj_ops->write(vol, cookie, buf, len, offset);
}

int vol_lookup(struct objstore_vol *vol, void *dircookie,
	       const char *name, struct noid *child)
{
	if (!vol || !name || !child)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->lookup)
		return -ENOTSUP;

	return vol->def->obj_ops->lookup(vol, dircookie, name, child);
}

int vol_create(struct objstore_vol *vol, void *dircookie, const char *name,
	       uint16_t mode, struct noid *child)
{
	if (!vol || !name || !child)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->create)
		return -ENOTSUP;

	return vol->def->obj_ops->create(vol, dircookie, name, mode, child);
}

int vol_unlink(struct objstore_vol *vol, void *dircookie, const char *name)
{
	if (!vol || !name)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->unlink)
		return -ENOTSUP;

	return vol->def->obj_ops->unlink(vol, dircookie, name);
}

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

	obj = umem_cache_alloc(obj_cache, 0);
	if (!obj)
		return NULL;

	memset(&obj->oid, 0, sizeof(obj->oid));

	avl_create(&obj->versions, ver_cmp, sizeof(struct objver),
		   offsetof(struct objver, node));

	obj->nversions = 0;
	obj->nlink = 0;
	obj->private = NULL;
	obj->open_cookie = NULL;
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

	mxdestroy(&obj->lock);
	vol_putref(obj->vol);
	avl_destroy(&obj->versions);
	umem_cache_free(obj_cache, obj);
}

/*
 * Allocate a new generic object version structure.
 */
struct objver *allocobjver(void)
{
	struct objver *ver;
	int ret;

	ver = umem_cache_alloc(objver_cache, 0);
	if (!ver)
		return ERR_PTR(-ENOMEM);

	ver->clock = nvclock_alloc();
	if (!ver->clock) {
		ret = -ENOMEM;
		goto err;
	}

	memset(&ver->attrs, 0, sizeof(ver->attrs));

	ver->private = NULL;
	ver->obj = NULL;

	return ver;

err:
	umem_cache_free(objver_cache, ver);

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
	umem_cache_free(objver_cache, ver);
}
