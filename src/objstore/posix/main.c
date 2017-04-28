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

#include "posix.h"

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

	/* create root object */
	ret = posix_new_obj(pv, NATTR_DIR | 0777, &pv->root);
	if (ret)
		goto err_paths;

	FIXME("flush vol info");

	vol->private = pv;
	vol->ops = &posix_vol_ops;

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
