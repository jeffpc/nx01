/*
 * Copyright (c) 2017-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include "posix.h"

#define OIDFMT	"%016"PRIx64

int posix_new_obj(struct posixvdev *pv, uint16_t mode, struct noid *oid)
{
	struct nvclock *clock;
	char vername[PATH_MAX];
	char dirname[20];
	uint64_t uniq;
	int ret;
	int fd;

	clock = nvclock_alloc(true);
	if (!clock)
		return -ENOMEM;

	ret = nvclock_to_str(clock, vername, sizeof(vername));
	if (ret)
		goto err_free_clock;

	ret = oidbmap_get_new(pv, &uniq);
	if (ret)
		goto err_free_clock;

	snprintf(dirname, sizeof(dirname), OIDFMT, uniq);

	ret = xmkdirat(pv->basefd, dirname, 0700);
	if (ret)
		goto err_free_uniq;

	fd = xopenat(pv->basefd, dirname, O_RDONLY, 0);
	if (fd < 0) {
		ret = fd;
		goto err_unlink_dir;
	}

	ret = xopenat(fd, vername, O_RDWR | O_CREAT, 0600);
	if (ret < 0)
		goto err_close;

	FIXME("write out obj attrs");

	xclose(ret);
	xclose(fd);

	nvclock_free(clock);

	noid_set(oid, &pv->volid, uniq);

	return 0;

err_close:
	xclose(fd);

err_unlink_dir:
	xunlinkat(pv->basefd, dirname, 0);

err_free_uniq:
	oidbmap_put(pv, uniq);

err_free_clock:
	nvclock_free(clock);

	return ret;
}

static int posix_getroot(struct objstore_vdev *vdev, struct noid *root)
{
	struct posixvdev *pv = vdev->private;

	*root = pv->root;

	return 0;
}

static int posix_allocobj(struct obj *obj)
{
	struct posixvdev *pv = obj->vol->vdev->private;
	char oidstr[32];
	int objfd;

	snprintf(oidstr, sizeof(oidstr), OIDFMT, obj->oid.uniq);

	objfd = xopenat(pv->basefd, oidstr, O_RDONLY, 0);
	ASSERT3S(objfd, >=, 0);

	FIXME("allocobj not fully implemented");

	return 0;
}

const struct vdev_ops posix_vdev_ops = {
	.getroot	= posix_getroot,
	.allocobj	= posix_allocobj,
};
