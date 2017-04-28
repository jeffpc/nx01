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
#include <jeffpc/io.h>
#include <jeffpc/rand.h>

#include <nomad/objstore_backend.h>

#include "posix.h"

#define OIDFMT	"%016"PRIx64

static int posix_getroot(struct objstore_vol *vol, struct noid *root)
{
	struct posixvol *pv = vol->private;

	*root = pv->root;

	return 0;
}

static int posix_allocobj(struct obj *obj)
{
	struct posixvol *pv = obj->vol->private;
	char oidstr[32];
	int objfd;

	snprintf(oidstr, sizeof(oidstr), OIDFMT, obj->oid.uniq);

	objfd = xopenat(pv->basefd, oidstr, O_RDONLY, 0);
	ASSERT3S(objfd, >=, 0);

	FIXME("allocobj not fully implemented");

	return 0;
}

static const struct vol_ops vol_ops = {
	.getroot	= posix_getroot,
	.allocobj	= posix_allocobj,
};

static int create_obj(struct posixvol *pv, uint64_t *uniq_r)
{
	char name[20];
	uint64_t uniq;
	int ret;
	int fd;

	ret = oidbmap_get_new(pv, &uniq);
	if (ret)
		return ret;

	snprintf(name, sizeof(name), OIDFMT, uniq);

	ret = xmkdirat(pv->basefd, name, 0700);
	if (ret)
		goto err_free_uniq;

	fd = xopenat(pv->basefd, name, O_RDONLY, 0);
	if (fd < 0) {
		ret = fd;
		goto err_unlink_dir;
	}

	FIXME("create obj files");

	xclose(fd);

	*uniq_r = uniq;

	return 0;

err_unlink_dir:
	xunlinkat(pv->basefd, name, 0);

err_free_uniq:
	oidbmap_put(pv, uniq);

	return ret;
}

static int create_root_obj(struct posixvol *pv)
{
	uint64_t uniq;
	int ret;

	ret = create_obj(pv, &uniq);
	if (ret)
		return ret;

	noid_set(&pv->root, pv->ds, uniq);

	return 0;
}

static int prep_paths(const char *base, struct posixvol *pv)
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

	pv->volfd = xopenat(pv->basefd, "vol", O_RDWR | O_CREAT, 0600);
	if (pv->volfd < 0) {
		ret = pv->volfd;
		goto err_basefd;
	}

	ret = oidbmap_create(pv);
	if (ret < 0)
		goto err_volfd;

	return 0;

err_volfd:
	xclose(pv->volfd);
	xunlinkat(pv->basefd, "vol", 0);

err_basefd:
	xclose(pv->basefd);

err_mkdir:
	xunlink(base);

	return ret;
}

static int posix_create(struct objstore_vol *vol)
{
	struct posixvol *pv;
	int ret;

	cmn_err(CE_WARN, "The POSIX objstore backend is still experimental");
	cmn_err(CE_WARN, "Do not expect compatibility from version to version");

	pv = malloc(sizeof(struct posixvol));
	if (!pv)
		return -ENOMEM;

	pv->ds = rand32();

	ret = prep_paths(vol->path, pv);
	if (ret)
		goto err_free;

	ret = create_root_obj(pv);
	if (ret)
		goto err_paths;

	FIXME("flush vol info");

	vol->private = pv;
	vol->ops = &vol_ops;

	return 0;

err_paths:
	FIXME("path cleanup is not yet implemented");

err_free:
	free(pv);

	return ret;
}

const struct objstore_vol_def objvol = {
	.name = "posix",

	.create = posix_create,
};
