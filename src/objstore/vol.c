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

int vol_getroot(struct objstore_vol *vol, struct noid *root)
{
	if (!vol || !root)
		return -EINVAL;

	if (!vol->def->vol_ops || !vol->def->vol_ops->getroot)
		return -ENOTSUP;

	return vol->def->vol_ops->getroot(vol, root);
}

int objstore_vol_create(struct objstore *vg, const char *path,
			enum objstore_mode mode)
{
	struct objstore_vol *s;
	int ret;

	if (!backend->def->vol_ops->create)
		return -ENOTSUP;

	s = umem_cache_alloc(vol_cache, 0);
	if (!s)
		return -ENOMEM;

	refcnt_init(&s->refcnt, 1);

	s->def = backend->def;
	s->mode = mode;
	s->path = strdup(path);
	if (!s->path) {
		ret = -ENOMEM;
		goto err;
	}

	ret = s->def->vol_ops->create(s);
	if (ret)
		goto err_path;

	/* hand off our reference */
	vg_add_vol(vg, s);

	return 0;

err_path:
	free((char *) s->path);

err:
	umem_cache_free(vol_cache, s);

	return ret;
}

int objstore_vol_load(struct objstore *vg, struct xuuid *uuid, const char *path)
{
	return -ENOTSUP;
}

void vol_free(struct objstore_vol *vol)
{
	if (!vol)
		return;

	free((char *) vol->path);
	umem_cache_free(vol_cache, vol);
}
