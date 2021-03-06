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
#define VOL_FILENAME	"vol"

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

static int prep_vdev(struct posixvdev *pv)
{
	int ret;

	ret = xmkdir(pv->vdev->path, 0700);
	if (ret)
		return ret;

	pv->basefd = xopen(pv->vdev->path, O_RDONLY, 0);
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
	xunlink(pv->vdev->path);

	return ret;
}

static int open_vdev(struct posixvdev *pvdev)
{
	char *tmp;
	int ret;

	pvdev->basefd = xopen(pvdev->vdev->path, O_RDONLY, 0);
	if (pvdev->basefd < 0)
		return pvdev->basefd;

	tmp = read_file_at(pvdev->basefd, VDEV_FILENAME);
	if (IS_ERR(tmp)) {
		ret = PTR_ERR(tmp);
		goto err;
	}

	if (!xuuid_parse(&pvdev->vdev->uuid, tmp)) {
		ret = -EINVAL;
		goto err_free;
	}

	return 0;

err_free:
	free(tmp);

err:
	xclose(pvdev->basefd);

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

	ret = prep_vdev(pv);
	if (ret)
		goto err_free;

	vdev->private = pv;

	return 0;

err_free:
	free(pv);

	return ret;
}

static int store_volinfo(struct posixvol *pvol)
{
	int ret;

	pvol->volfd = xopenat(pvol->basefd, VOL_FILENAME, O_RDWR | O_CREAT,
			      0600);
	if (pvol->volfd < 0)
		return pvol->volfd;

	ret = oidbmap_create(pvol);
	if (ret)
		goto err;

	return 0;

err:
	xclose(pvol->volfd);
	xunlinkat(pvol->basefd, VOL_FILENAME, 0);

	return ret;
}

static int prep_vol(const char *volid, struct posixvol *pvol)
{
	struct posixvdev *pv = pvol->vol->vdev->private;
	int ret;

	ret = xmkdirat(pv->basefd, volid, 0700);
	if (ret)
		return ret;

	pvol->basefd = xopenat(pv->basefd, volid, O_RDONLY, 0);
	if (pvol->basefd < 0) {
		ret = pvol->basefd;
		goto err_mkdir;
	}

	ret = store_volinfo(pvol);
	if (ret)
		goto err_basefd;

	return 0;

err_basefd:
	xclose(pvol->basefd);

err_mkdir:
	xunlinkat(pv->basefd, volid, 0);

	return ret;
}

static int posix_create_vol(struct objstore *vol)
{
	char volid[XUUID_PRINTABLE_STRING_LENGTH];
	struct posixvol *pvol;
	int ret;

	pvol = malloc(sizeof(struct posixvol));
	if (!pvol)
		return -ENOMEM;

	pvol->vol = vol;

	xuuid_unparse(&vol->id, volid);

	ret = prep_vol(volid, pvol);
	if (ret)
		goto err_free;

	/* create the root */
	ret = posix_new_obj(pvol, NATTR_DIR | 0777, &pvol->root);
	if (ret)
		goto err_paths;

	/* TODO: store root oid so we know the root later on */

	vol->ops = &posix_vol_ops;
	vol->private = pvol;

	return 0;

err_paths:
	FIXME("remove partially constructed volume files & dirs");

err_free:
	free(pvol);

	return ret;
}

static int posix_load(struct objstore_vdev *vdev)
{
	struct posixvdev *pv;
	int ret;

	cmn_err(CE_WARN, "The POSIX objstore backend is still experimental");
	cmn_err(CE_WARN, "Do not expect compatibility from version to version");

	pv = malloc(sizeof(struct posixvdev));
	if (!pv)
		return -ENOMEM;

	pv->vdev = vdev;

	ret = open_vdev(pv);
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
	.create_vol = posix_create_vol,
	.load = posix_load,
};
