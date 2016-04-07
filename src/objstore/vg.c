/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <stddef.h>
#include <umem.h>

#include <jeffpc/error.h>

#include <nomad/iter.h>
#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

static umem_cache_t *vg_cache;

static pthread_mutex_t vgs_lock;
static list_t vgs;

int vg_init(void)
{
	struct objstore *filecache;

	vg_cache = umem_cache_create("vg", sizeof(struct objstore),
				     0, NULL, NULL, NULL, NULL, NULL, 0);
	if (!vg_cache)
		return -ENOMEM;

	mxinit(&vgs_lock);

	list_create(&vgs, sizeof(struct objstore),
		    offsetof(struct objstore, node));

	filecache = objstore_vg_create("file$");
	if (IS_ERR(filecache)) {
		umem_cache_destroy(vg_cache);
		return PTR_ERR(filecache);
	}

	return 0;
}

struct objstore *objstore_vg_create(const char *name)
{
	struct objstore *vg;

	vg = umem_cache_alloc(vg_cache, 0);
	if (!vg)
		return ERR_PTR(-ENOMEM);

	vg->name = strdup(name);
	if (!vg->name) {
		umem_cache_free(vg_cache, vg);
		return ERR_PTR(-ENOMEM);
	}

	vg->vol = NULL;

	mxinit(&vg->lock);

	mxlock(&vgs_lock);
	list_insert_tail(&vgs, vg);
	mxunlock(&vgs_lock);

	return vg;
}

void vg_add_vol(struct objstore *vg, struct objstore_vol *vol)
{
	mxlock(&vg->lock);
	ASSERT3P(vg->vol, ==, NULL);
	vg->vol = vol;
	mxunlock(&vg->lock);
}

struct objstore *objstore_vg_lookup(const char *name)
{
	struct objstore *vg;

	mxlock(&vgs_lock);
	list_for_each(&vgs, vg)
		if (!strcmp(name, vg->name))
			break;
	mxunlock(&vgs_lock);

	return vg;
}

int objstore_getroot(struct objstore *vg, struct noid *root)
{
	int ret;

	if (!vg || !root)
		return -EINVAL;

	mxlock(&vg->lock);
	if (vg->vol)
		ret = vol_getroot(vg->vol, root);
	else
		ret = -ENXIO;
	mxunlock(&vg->lock);

	return ret;
}

int objstore_getattr(struct objstore *vg, const struct noid *oid,
		     const struct nvclock *clock, struct nattr *attr)
{
	int ret;

	if (!vg || !oid || !clock || !attr)
		return -EINVAL;

	mxlock(&vg->lock);
	if (vg->vol)
		ret = vol_getattr(vg->vol, oid, clock, attr);
	else
		ret = -ENXIO;
	mxunlock(&vg->lock);

	return ret;
}

int objstore_lookup(struct objstore *vg, const struct noid *dir_oid,
		    const struct nvclock *dir_clock, const char *name,
		    struct noid *child)
{
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, %p, '%s', %p)", __func__, vg, dir_oid,
		dir_clock, name, child);

	if (!vg || !dir_oid || !dir_clock || !name || !child)
		return -EINVAL;

	mxlock(&vg->lock);
	if (vg->vol)
		ret = vol_lookup(vg->vol, dir_oid, dir_clock, name, child);
	else
		ret = -ENXIO;
	mxunlock(&vg->lock);

	return ret;
}

int objstore_create(struct objstore *vg, const struct noid *dir_oid,
		    const struct nvclock *dir_clock, const char *name,
		    uint16_t mode, struct noid *child)
{
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, %p, '%s', %#o, %p)", __func__, vg,
		dir_oid, dir_clock, name, mode, child);

	if (!vg || !dir_oid || !dir_clock || !name || !child)
		return -EINVAL;

	mxlock(&vg->lock);
	if (vg->vol)
		ret = vol_create(vg->vol, dir_oid, dir_clock, name, mode,
				 child);
	else
		ret = -ENXIO;
	mxunlock(&vg->lock);

	return ret;
}

int objstore_remove(struct objstore *vg, const struct noid *dir_oid,
		    const struct nvclock *dir_clock, const char *name)
{
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, %p, '%s')", __func__, vg, dir_oid,
		dir_clock, name);

	if (!vg || !dir_oid || !dir_clock || !name)
		return -EINVAL;

	mxlock(&vg->lock);
	if (vg->vol)
		ret = vol_remove(vg->vol, dir_oid, dir_clock, name);
	else
		ret = -ENXIO;
	mxunlock(&vg->lock);

	return ret;
}
