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

#include <jeffpc/error.h>

#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

int objstore_vdev_create(struct objstore *vol, const char *type,
			const char *path)
{
	struct objstore_vdev *vdev;
	struct backend *backend;
	int ret;

	backend = backend_lookup(type);
	if (!backend || !backend->def->create)
		return -ENOTSUP;

	vdev = mem_cache_alloc(vdev_cache);
	if (!vdev)
		return -ENOMEM;

	refcnt_init(&vdev->refcnt, 1);

	vdev->vol = vol;
	vdev->def = backend->def;
	vdev->path = strdup(path);
	if (!vdev->path) {
		ret = -ENOMEM;
		goto err;
	}

	ret = vdev->def->create(vdev);
	if (ret)
		goto err_path;

	/* hand off our reference */
	vol_add_vdev(vol, vdev);

	return 0;

err_path:
	free((char *) vdev->path);

err:
	mem_cache_free(vdev_cache, vdev);

	return ret;
}

int objstore_vdev_load(struct objstore *vol, struct xuuid *uuid,
		      const char *path)
{
	return -ENOTSUP;
}

void vdev_free(struct objstore_vdev *vdev)
{
	if (!vdev)
		return;

	free((char *) vdev->path);
	mem_cache_free(vdev_cache, vdev);
}
