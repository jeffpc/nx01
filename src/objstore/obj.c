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

#include <jeffpc/error.h>

#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

void *vol_open(struct objstore_vol *vol, const struct noid *oid,
	       const struct nvclock *clock)
{
	if (!vol || !oid || !clock)
		return ERR_PTR(-EINVAL);

	if (!vol->def->obj_ops || !vol->def->obj_ops->open)
		return ERR_PTR(-ENOTSUP);

	return vol->def->obj_ops->open(vol, oid, clock);
}

int vol_close(struct objstore_vol *vol, void *cookie)
{
	if (!vol)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->close)
		return -ENOTSUP;

	return vol->def->obj_ops->close(vol, cookie);
}

int vol_getattr(struct objstore_vol *vol, void *cookie, struct nattr *attr)
{
	if (!vol || !attr)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->getattr)
		return -ENOTSUP;

	return vol->def->obj_ops->getattr(vol, cookie, attr);
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

int vol_remove(struct objstore_vol *vol, void *dircookie, const char *name)
{
	if (!vol || !name)
		return -EINVAL;

	if (!vol->def->obj_ops || !vol->def->obj_ops->remove)
		return -ENOTSUP;

	return vol->def->obj_ops->remove(vol, dircookie, name);
}
