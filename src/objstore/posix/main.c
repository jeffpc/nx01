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

#define VDEV_FILENAME	"vdev"

static int store_vdevid(struct posixvdev *pv)
{
	char vdevid[XUUID_PRINTABLE_STRING_LENGTH];
	int fd;
	int ret;

	xuuid_unparse(&pv->vdev->uuid, vdevid);

	fd = xopenat(pv->basefd, VDEV_FILENAME, O_RDWR | O_CREAT, 0600);
	if (fd < 0)
		return fd;

	ret = xwrite_str(fd, vdevid);

	xclose(fd);

	if (ret)
		xunlinkat(pv->basefd, VDEV_FILENAME, 0);

	return ret;
}

static int prep_vdev(const char *base, struct posixvdev *pv)
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

	ret = store_vdevid(pv);
	if (ret)
		goto err_basefd;

	return 0;

err_basefd:
	xunlinkat(pv->basefd, VDEV_FILENAME, 0);

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

	pv->vdev = vdev;
	pv->vdevfd = -1;

	ret = prep_vdev(vdev->path, pv);
	if (ret)
		goto err_free;

	vdev->private = pv;

	return 0;

err_free:
	free(pv);

	return ret;
}

const struct objstore_vdev_def objvdev = {
	.name = "posix",

	.create = posix_create,
};
