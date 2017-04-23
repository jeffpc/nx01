/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * Copyright (c) 2016 Holly Sipek
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

#define MAX_OID_UNIQ	(1ull << 32)
#define OID_BMAP_SIZE	(MAX_OID_UNIQ / 8)

static int oid2str(char *dest, size_t len, struct noid *oid)
{
	return snprintf(dest, len, "%08x-%016"PRIx64, oid->ds, oid->uniq);
}

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
	int ret;

	oid2str(oidstr, sizeof(oidstr), &obj->oid);

	objfd = xopenat(pv->basefd, oidstr, O_RDONLY, 0);
	ASSERT3S(objfd, >=, 0);

	panic("posix_allocobj not supported! %d", ENOTSUP);

	/* FIXME: this is not implemented yet! */

	return ret;
}

static const struct vol_ops vol_ops = {
	.getroot	= posix_getroot,
	.allocobj	= posix_allocobj,
};

static int oidbmap_set(struct posixvol *pv, uint64_t uniq)
{
	const off_t off = uniq / 8;
	const uint8_t bit = 1 << (uniq % 8);
	uint8_t tmp;
	int ret;

	mxlock(&pv->lock);

	ret = xpread(pv->oidbmap, &tmp, sizeof(tmp), off);

	if (ret)
		goto out;

	tmp |= bit;

	ret = xpwrite(pv->oidbmap, &tmp, sizeof(tmp), off);

out:
	mxunlock(&pv->lock);

	return ret;
}

static int oidbmap_get_new(struct posixvol *pv, uint64_t *new)
{
	off_t off;
	int ret;

	for (off = 0; off < OID_BMAP_SIZE; off++) {
		uint8_t tmp;
		int bitno;

		ret = xpread(pv->oidbmap, &tmp, sizeof(tmp), off);
		if (ret)
			return ret;

		/* all full */
		if (tmp == 0xff)
			continue;

		for (bitno = 0; bitno < 8; bitno++)
			if (!((1 << bitno) & tmp))
				break;

		if (bitno == 8)
			continue;

		tmp |= 1 << bitno;

		ret = xpwrite(pv->oidbmap, &tmp, sizeof(tmp), off);
		if (ret)
			return ret;

		*new = (off * 8) + bitno;

		cmn_err(CE_INFO, "%s allocating %"PRIu64, __func__, *new);

		return 0;
	}

	return -ENOSPC;
}

static int posix_write_vers(struct obj *obj, int obj_fd)
{
	struct posixobj *po = obj->private;
	struct posix_obj_vers *vers = &po->vers;

	/* FIXME: Actually write the versions information to the file */

	return 0;
}

static int posix_write_attr(struct obj *obj, int obj_fd)
{
	struct posixobj *po = obj->private;
	struct posix_obj_vers_attr *attr = &po->attr;

	/* FIXME: Actually write the attr information to the file */

	return 0;
}

static int create_oid_files(struct obj *obj, int obj_fd)
{
	int ret;
	int fd;

	fd = xopenat(obj_fd, "vers", O_RDWR | O_CREAT, 0600);
	ret = posix_write_vers(obj, fd);
	if (ret)
		return ret;

	xclose(fd);

	/* FIXME: don't hardcode the creation sequence number as 1. */
	/* FIXME: stash the highest sequence number for the object somewhere. */

	fd = xopenat(obj_fd, "1.attr", O_RDWR | O_CREAT, 0600);
	ret = posix_write_attr(obj, fd);
	if (ret)
		return ret;

	xclose(fd);

	fd = xopenat(obj_fd, "1", O_RDWR | O_CREAT, 0600);
	xclose(fd);

	return 0;
}

static int create_root_obj(struct posixvol *pv)
{
	struct obj *root;
	uint64_t uniq;
	char dirname[32];
	int objdir_fd;
	int ret;

	ret = oidbmap_get_new(pv, &uniq);
	if (ret)
		return ret;

	noid_set(&pv->root, pv->ds, uniq);

	oid2str(dirname, sizeof(dirname), &pv->root);

	/* write a stringified version of the root object ID to the vol file */
	/* FIXME: make a better format for the vol file */
	ret = xwrite_str(pv->volfd, dirname);
	if (ret)
		return ret;

	ret = mkdirat(pv->basefd, dirname, 0700);
	if (ret)
		return ret;

	objdir_fd = openat(pv->basefd, dirname, O_RDONLY, 0);

	root = malloc(sizeof(struct obj));
	if (!root)
		return -ENOMEM;

	root->oid = pv->root;
	/* FIXME: fill in the rest of the root object */

	ret = create_oid_files(root, objdir_fd);

	/* FIXME: clean up everything on failures. */
	return ret;
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

	pv->oidbmap = xopenat(pv->basefd, "oid-bmap", O_RDWR | O_CREAT, 0600);
	if (pv->oidbmap < 0) {
		ret = pv->oidbmap;
		goto err_volfd;
	}

	ret = xftruncate(pv->oidbmap, OID_BMAP_SIZE);
	if (ret)
		goto err_oidbmap;

	/* reserve 0 since it is illegal to use it */
	ret = oidbmap_set(pv, 0);
	if (ret)
		goto err_oidbmap;

	return 0;

err_oidbmap:
	xclose(pv->oidbmap);
	xunlinkat(pv->basefd, "oid-bmap", 0);

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

	mxinit(&pv->lock);

	pv->ds = rand32();

	ret = prep_paths(vol->path, pv);
	if (ret)
		goto err_free;

	ret = create_root_obj(pv);
	if (ret)
		goto err_paths;

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
