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

static int objcmp(const void *va, const void *vb)
{
	const struct obj *a = va;
	const struct obj *b = vb;

	return noid_cmp(&a->oid, &b->oid);
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
	avl_create(&vg->objs, objcmp, sizeof(struct obj),
		   offsetof(struct obj, node));

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

static struct objstore_vol *findvol(struct objstore *vg)
{
	struct objstore_vol *vol;

	mxlock(&vg->lock);
	vol = vol_getref(vg->vol);
	mxunlock(&vg->lock);

	return vol;
}

/*
 * Find the object with oid, if there isn't one, allocate one and add it to
 * the vg's list of objects.
 *
 * Returns obj (locked and referenced) on success, negated errno on failure.
 */
static struct obj *__find_or_alloc(struct objstore *vg, struct objstore_vol *vol,
				   const struct noid *oid)
{
	struct obj key = {
		.oid = *oid,
	};
	struct obj *obj, *newobj;
	avl_index_t where;
	bool inserted;

	inserted = false;
	newobj = NULL;

	for (;;) {
		mxlock(&vg->lock);

		/* try to find the object */
		obj = obj_getref(avl_find(&vg->objs, &key, &where));

		/* not found and this is the second attempt -> insert it */
		if (!obj && newobj) {
			avl_insert(&vg->objs, obj_getref(newobj), where);
			obj = newobj;
			newobj = NULL;
			inserted = true;
		}

		mxunlock(&vg->lock);

		/* found or inserted -> we're done */
		if (obj) {
			/* newly inserted objects are already locked */
			if (!inserted)
				mxlock(&obj->lock);

			if (obj->state == OBJ_STATE_DEAD) {
				/* this is a dead one, try again */
				mxunlock(&obj->lock);
				obj_putref(obj);
				continue;
			}

			if (newobj) {
				mxunlock(&newobj->lock);
				obj_putref(newobj);
			}

			break;
		}

		/* allocate a new object */
		newobj = allocobj();
		if (!newobj)
			return ERR_PTR(-ENOMEM);

		mxlock(&newobj->lock);

		newobj->oid = *oid;
		newobj->vol = vol_getref(vol);

		/* retry the search, and insert if necessary */
	}

	return obj;
}

/*
 * Given a vg and an oid, find the corresponding object structure.
 *
 * Return with the object locked and referenced.
 */
static struct obj *getobj(struct objstore *vg, const struct noid *oid)
{
	struct objstore_vol *vol;
	struct obj *obj;
	int ret;

	vol = findvol(vg);
	if (!vol)
		return ERR_PTR(-ENXIO);

	obj = __find_or_alloc(vg, vol, oid);
	if (IS_ERR(obj)) {
		ret = PTR_ERR(obj);
		goto err;
	}

	switch (obj->state) {
		case OBJ_STATE_NEW:
			if (vol->ops && vol->ops->allocobj &&
			    (ret = vol->ops->allocobj(obj))) {
				/* the allocobj op failed, mark the obj dead */
				obj->state = OBJ_STATE_DEAD;

				/* remove the object from the objs list */
				mxlock(&vg->lock);
				avl_remove(&vg->objs, obj);
				mxunlock(&vg->lock);
				goto err_obj;
			}

			obj->state = OBJ_STATE_LIVE;
			break;
		case OBJ_STATE_LIVE:
			break;
		case OBJ_STATE_DEAD:
			ret = -EINVAL;
			goto err_obj;
	}

	vol_putref(vol);

	return obj;

err_obj:
	mxunlock(&obj->lock);
	obj_putref(obj);

err:
	vol_putref(vol);

	return ERR_PTR(ret);
}

/*
 * Given a vg, an oid, and a vector clock, find the corresponding object
 * version structure.
 *
 * Return the found version, with the object referenced and locked.
 */
static struct objver *getver(struct objstore *vg, const struct noid *oid,
			     const struct nvclock *clock)
{
	struct objver *ver;
	struct obj *obj;

	/*
	 * First, find the object based on the oid.
	 */
	obj = getobj(vg, oid);
	if (IS_ERR(obj))
		return ERR_CAST(obj);

	/*
	 * Second, find the right version of the object.
	 */
	if (obj->nversions == 0) {
		/*
		 * There are no versions at all.
		 */
		ver = ERR_PTR(-ENOENT);
	} else if (!nvclock_is_null(clock)) {
		/*
		 * We are looking for a specific version.  Since the
		 * obj->objects AVL tree is only a cache, it may not contain
		 * it.  If it doesn't we need to call into the backend to
		 * fetch it.
		 */
		struct objver key = {
			.clock = (struct nvclock *) clock,
		};

		ver = avl_find(&obj->versions, &key, NULL);
		if (ver)
			return ver;

		/*
		 * TODO: try to fetch the version from the backend
		 */
		ver = ERR_PTR(-ENOTSUP);
		if (!IS_ERR(ver)) {
			avl_add(&obj->versions, ver);
			return ver;
		}
	} else if (obj->nversions == 1) {
		/*
		 * We are *not* looking for a specific version, and there is
		 * only one version available, so we just return that.  This
		 * is slightly complicated by the fact that the
		 * obj->versions AVL tree is only a cache, and therefore it
		 * might be empty.  If it is, we need to call into the
		 * backend to fetch it.
		 */
		ver = avl_first(&obj->versions);
		if (ver)
			return ver;

		/*
		 * TODO: get the only version
		 */
		ver = ERR_PTR(-ENOTSUP);
		if (!IS_ERR(ver)) {
			avl_add(&obj->versions, ver);
			return ver;
		}
	} else {
		/*
		 * We are *not* looking for a specific version, and there
		 * are two or more versions.
		 */
		ver = ERR_PTR(-ENOTUNIQ);
	}

	mxunlock(&obj->lock);
	obj_putref(obj);
	return ver;
}

int objstore_getroot(struct objstore *vg, struct noid *root)
{
	struct objstore_vol *vol;
	int ret;

	if (!vg || !root)
		return -EINVAL;

	vol = findvol(vg);
	if (!vol)
		return -ENXIO;

	if (vol->ops && vol->ops->getroot)
		ret = vol->ops->getroot(vol, root);
	else
		ret = -ENOTSUP;

	vol_putref(vol);

	return ret;
}

void *objstore_open(struct objstore *vg, const struct noid *oid,
		    const struct nvclock *clock)
{
	struct objstore_vol *vol;
	void *cookie;

	if (!vg || !oid || !clock)
		return ERR_PTR(-EINVAL);

	vol = findvol(vg);
	if (!vol)
		return ERR_PTR(-ENXIO);

	cookie = vol_open(vol, oid, clock);

	vol_putref(vol);

	return cookie;
}

int objstore_close(struct objstore *vg, void *cookie)
{
	struct objstore_vol *vol;
	int ret;

	if (!vg)
		return -EINVAL;

	vol = findvol(vg);
	if (!vol)
		return -ENXIO;

	ret = vol_close(vol, cookie);

	vol_putref(vol);

	return ret;
}

int objstore_getattr(struct objstore *vg, void *cookie, struct nattr *attr)
{
	struct objstore_vol *vol;
	int ret;

	if (!vg || !attr)
		return -EINVAL;

	vol = findvol(vg);
	if (!vol)
		return -ENXIO;

	ret = vol_getattr(vol, cookie, attr);

	vol_putref(vol);

	return ret;
}

int objstore_setattr(struct objstore *vg, void *cookie, const struct nattr *attr,
		     const unsigned valid)
{
	struct objstore_vol *vol;
	int ret;

	if (!vg || !attr)
		return -EINVAL;

	vol = findvol(vg);
	if (!vol)
		return -ENXIO;

	if (!valid)
		ret = 0;
	else
		ret = vol_setattr(vol, cookie, attr, valid);

	vol_putref(vol);

	return ret;
}

ssize_t objstore_read(struct objstore *vg, void *cookie, void *buf, size_t len,
		      uint64_t offset)
{
	struct objstore_vol *vol;
	ssize_t ret;

	if (!vg || !buf)
		return -EINVAL;

	if (len > (SIZE_MAX / 2))
		return -EOVERFLOW;

	vol = findvol(vg);
	if (!vol)
		return -ENXIO;

	ret = vol_read(vol, cookie, buf, len, offset);

	vol_putref(vol);

	return ret;
}

ssize_t objstore_write(struct objstore *vg, void *cookie, const void *buf,
		       size_t len, uint64_t offset)
{
	struct objstore_vol *vol;
	ssize_t ret;

	if (!vg || !buf)
		return -EINVAL;

	if (len > (SIZE_MAX / 2))
		return -EOVERFLOW;

	vol = findvol(vg);
	if (!vol)
		return -ENXIO;

	ret = vol_write(vol, cookie, buf, len, offset);

	vol_putref(vol);

	return ret;
}

int objstore_lookup(struct objstore *vg, void *dircookie, const char *name,
		    struct noid *child)
{
	struct objstore_vol *vol;
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, '%s', %p)", __func__, vg, dircookie,
		name, child);

	if (!vg || !name || !child)
		return -EINVAL;

	vol = findvol(vg);
	if (!vol)
		return -ENXIO;

	ret = vol_lookup(vol, dircookie, name, child);

	vol_putref(vol);

	return ret;
}

int objstore_create(struct objstore *vg, void *dircookie, const char *name,
		    uint16_t mode, struct noid *child)
{
	struct objstore_vol *vol;
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, '%s', %#o, %p)", __func__, vg,
		dircookie, name, mode, child);

	if (!vg || !name || !child)
		return -EINVAL;

	vol = findvol(vg);
	if (!vol)
		return -ENXIO;

	ret = vol_create(vol, dircookie, name, mode, child);

	vol_putref(vol);

	return ret;
}

int objstore_unlink(struct objstore *vg, void *dircookie, const char *name)
{
	struct objstore_vol *vol;
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, '%s')", __func__, vg, dircookie, name);

	if (!vg || !name)
		return -EINVAL;

	vol = findvol(vg);
	if (!vol)
		return -ENXIO;

	ret = vol_unlink(vol, dircookie, name);

	vol_putref(vol);

	return ret;
}
