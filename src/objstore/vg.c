/*
 * Copyright (c) 2015 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <nomad/error.h>
#include <nomad/iter.h>
#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

static pthread_mutex_t vgs_lock;
static list_t vgs;

int vg_init(void)
{
	struct objstore *filecache;

	mxinit(&vgs_lock);

	list_create(&vgs, sizeof(struct objstore),
		    offsetof(struct objstore, node));

	filecache = objstore_vg_create("file$", OS_VG_SIMPLE);
	if (IS_ERR(filecache))
		return PTR_ERR(filecache);

	return 0;
}

struct objstore *objstore_vg_create(const char *name,
				    enum objstore_vg_type type)
{
	struct objstore *vg;

	if (type != OS_VG_SIMPLE)
		return ERR_PTR(EINVAL);

	vg = malloc(sizeof(struct objstore));
	if (!vg)
		return ERR_PTR(ENOMEM);

	vg->name = strdup(name);
	if (!vg->name) {
		free(vg);
		return ERR_PTR(ENOMEM);
	}

	list_create(&vg->vols, sizeof(struct objstore_vol),
		    offsetof(struct objstore_vol, vg_list));

	mxinit(&vg->lock);

	mxlock(&vgs_lock);
	list_insert_tail(&vgs, vg);
	mxunlock(&vgs_lock);

	return vg;
}

void vg_add_vol(struct objstore *vg, struct objstore_vol *vol)
{
	mxlock(&vg->lock);
	list_insert_tail(&vg->vols, vol);
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

int objstore_getroot(struct objstore *vg, struct nobjhndl *hndl)
{
	struct objstore_vol *vol;
	int ret;

	if (!vg || !hndl)
		return EINVAL;

	/*
	 * TODO: we're assuming OS_VG_SIMPLE
	 */
	mxlock(&vg->lock);
	vol = list_head(&vg->vols);
	if (vol)
		ret = vol_getroot(vol, hndl);
	else
		ret = ENXIO;
	mxunlock(&vg->lock);

	return ret;
}

int objstore_getattr(struct objstore *vg, const struct nobjhndl *hndl,
		     struct nattr *attr)
{
	struct objstore_vol *vol;
	int ret;

	if (!vg || !hndl || !attr)
		return EINVAL;

	/*
	 * TODO: we're assuming OS_VG_SIMPLE
	 */
	mxlock(&vg->lock);
	vol = list_head(&vg->vols);
	if (vol)
		ret = vol_getattr(vol, hndl, attr);
	else
		ret = ENXIO;
	mxunlock(&vg->lock);

	return ret;
}

int objstore_lookup(struct objstore *vg, const struct nobjhndl *dir,
		    const char *name, struct nobjhndl *child)
{
	struct objstore_vol *vol;
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, '%s', %p)", __func__, vg, dir, name,
		child);

	if (!vg || !dir || !name || !child)
		return EINVAL;

	/*
	 * TODO: we're assuming OS_VG_SIMPLE
	 */
	mxlock(&vg->lock);
	vol = list_head(&vg->vols);
	if (vol)
		ret = vol_lookup(vol, dir, name, child);
	else
		ret = ENXIO;
	mxunlock(&vg->lock);

	return ret;
}

int objstore_create(struct objstore *vg, const struct nobjhndl *dir,
                    const char *name, uint16_t mode,
                    struct nobjhndl *child)
{
	struct objstore_vol *vol;
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, '%s', %#o, %p)", __func__, vg, dir,
		name, mode, child);

	if (!vg || !dir || !name || !child)
		return EINVAL;

	/*
	 * TODO: we're assuming OS_VG_SIMPLE
	 */
	mxlock(&vg->lock);
	vol = list_head(&vg->vols);
	if (vol)
		ret = vol_create(vol, dir, name, mode, child);
	else
		ret = ENXIO;
	mxunlock(&vg->lock);

	return ret;
}

int objstore_remove(struct objstore *vg, const struct nobjhndl *dir,
                    const char *name)
{
	struct objstore_vol *vol;
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, '%s')", __func__, vg, dir, name);

	if (!vg || !dir || !name)
		return EINVAL;

	/*
	 * TODO: we're assuming OS_VG_SIMPLE
	 */
	mxlock(&vg->lock);
	vol = list_head(&vg->vols);
	if (vol)
		ret = vol_remove(vol, dir, name);
	else
		ret = ENXIO;
	mxunlock(&vg->lock);

	return ret;
}
