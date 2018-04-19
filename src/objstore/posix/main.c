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
#include <jeffpc/io.h>
#include <jeffpc/rand.h>

#include "posix.h"

static int prep_paths(const char *base, struct posixvdev *pv)
{
	int ret;

	ret = xmkdir(base, 0700);
	if (ret)
		return ret;

	pv->basefd = xopen(base, O_RDONLY, 0);
	if (pv->basefd < 0) {
		ret = pv->basefd;
		goto err_mkdir;
	}

	pv->vdevfd = xopenat(pv->basefd, "vdev", O_RDWR | O_CREAT, 0600);
	if (pv->vdevfd < 0) {
		ret = pv->vdevfd;
		goto err_basefd;
	}

	ret = oidbmap_create(pv);
	if (ret < 0)
		goto err_vdevfd;

	return 0;

err_vdevfd:
	xclose(pv->vdevfd);
	xunlinkat(pv->basefd, "vdev", 0);

err_basefd:
	xclose(pv->basefd);

err_mkdir:
	xunlink(base);

	return ret;
}

static int posix_create(struct objstore_vdev *vdev)
{
	struct posixvdev *pv;
	int ret;

	cmn_err(CE_WARN, "The POSIX objstore backend is still experimental");
	cmn_err(CE_WARN, "Do not expect compatibility from version to version");

	pv = malloc(sizeof(struct posixvdev));
	if (!pv)
		return -ENOMEM;

	xuuid_generate(&pv->volid);
	pv->vdev = vdev;

	ret = prep_paths(vdev->path, pv);
	if (ret)
		goto err_free;

	/* create root object */
	ret = posix_new_obj(pv, NATTR_DIR | 0777, &pv->root);
	if (ret)
		goto err_paths;

	FIXME("flush vdev info");

	vdev->private = pv;
	vdev->ops = &posix_vdev_ops;

	return 0;

err_paths:
	FIXME("path cleanup is not yet implemented");

err_free:
	free(pv);

	return ret;
}

const struct objstore_vdev_def objvdev = {
	.name = "posix",

	.create = posix_create,
};
