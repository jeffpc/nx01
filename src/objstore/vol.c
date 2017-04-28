/*
 * Copyright (c) 2015-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

int objstore_vol_create(struct objstore *vg, const char *path,
			enum objstore_mode mode)
{
	struct objstore_vol *vol;
	struct backend *backend;
	int ret;

	backend = backend_lookup("mem");
	if (!backend || !backend->def->create)
		return -ENOTSUP;

	vol = mem_cache_alloc(vol_cache);
	if (!vol)
		return -ENOMEM;

	refcnt_init(&vol->refcnt, 1);

	vol->vg = vg;
	vol->def = backend->def;
	vol->mode = mode;
	vol->path = strdup(path);
	if (!vol->path) {
		ret = -ENOMEM;
		goto err;
	}

	ret = vol->def->create(vol);
	if (ret)
		goto err_path;

	/* hand off our reference */
	vg_add_vol(vg, vol);

	return 0;

err_path:
	free((char *) vol->path);

err:
	mem_cache_free(vol_cache, vol);

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
	mem_cache_free(vol_cache, vol);
}
