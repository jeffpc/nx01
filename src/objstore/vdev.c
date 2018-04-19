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

static struct mem_cache *vdev_cache;

static struct lock vdevs_lock;
static struct list vdevs;

int vdev_init(void)
{
	vdev_cache = mem_cache_create("vdev", sizeof(struct objstore), 0);
	if (IS_ERR(vdev_cache))
		return PTR_ERR(vdev_cache);

	mxinit(&vdevs_lock);

	list_create(&vdevs, sizeof(struct objstore_vdev),
		    offsetof(struct objstore_vdev, node));

	return 0;
}

void vdev_fini(void)
{
	list_destroy(&vdevs);

	mxdestroy(&vdevs_lock);

	mem_cache_destroy(vdev_cache);
}

static struct objstore_vdev *vdev_alloc(const char *type, const char *path)
{
	struct objstore_vdev *vdev;
	struct backend *backend;
	int ret;

	backend = backend_lookup(type);
	if (!backend || !backend->def->create)
		return ERR_PTR(-ENOTSUP);

	vdev = mem_cache_alloc(vdev_cache);
	if (!vdev)
		return ERR_PTR(-ENOMEM);

	refcnt_init(&vdev->refcnt, 1);

	vdev->def = backend->def;
	vdev->private = NULL;

	vdev->path = strdup(path);
	if (!vdev->path) {
		ret = -ENOMEM;
		goto err;
	}

	return vdev;

err:
	vdev_putref(vdev);

	return ERR_PTR(ret);
}

void objstore_vdev_free(struct objstore_vdev *vdev)
{
	if (!vdev)
		return;

	free((char *) vdev->path);
	mem_cache_free(vdev_cache, vdev);
}

static struct objstore_vdev *vdev_load(const char *type, const char *path,
				       bool create)
{
	int (*fxn)(struct objstore_vdev *vdev);
	struct objstore_vdev *vdev;
	int ret;

	vdev = vdev_alloc(type, path);
	if (IS_ERR(vdev))
		return vdev;

	if (create) {
		fxn = vdev->def->create;
		xuuid_generate(&vdev->uuid);
	} else {
		fxn = vdev->def->load;
		xuuid_clear(&vdev->uuid);
	}

	ret = fxn(vdev);
	if (ret)
		goto err;

	mxlock(&vdevs_lock);
	list_insert_tail(&vdevs, vdev_getref(vdev));
	mxunlock(&vdevs_lock);

	return vdev;

err:
	vdev_putref(vdev);

	return ERR_PTR(ret);
}

struct objstore_vdev *objstore_vdev_create(const char *type, const char *path)
{
	return vdev_load(type, path, true);
}

struct objstore_vdev *objstore_vdev_load(const char *type, const char *path)
{
	return vdev_load(type, path, false);
}
