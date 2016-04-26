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
 * Fetch a version from the backend, adding it to the cached versions tree.
 */
static struct objver *__fetch_ver(struct obj *obj, const struct nvclock *clock)
{
	struct objver *ver;
	int ret;

	if (!obj || !obj->ops || !obj->ops->getversion)
		return ERR_PTR(-ENOTSUP);

	ver = allocobjver();
	if (IS_ERR(ver))
		return ver;

	if (clock)
		nvclock_copy(ver->clock, clock);

	ver->obj = obj;

	ret = obj->ops->getversion(ver);
	if (ret) {
		freeobjver(ver);
		return ERR_PTR(ret);
	}

	avl_add(&obj->versions, ver);

	return ver;
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

		/* check the cache */
		ver = avl_find(&obj->versions, &key, NULL);
		if (ver)
			return ver;

		/* try to fetch the version from the backend */
		ver = __fetch_ver(obj, clock);
		if (!IS_ERR(ver))
			return ver;
	} else if (obj->nversions == 1) {
		/*
		 * We are *not* looking for a specific version, and there is
		 * only one version available, so we just return that.  This
		 * is slightly complicated by the fact that the
		 * obj->versions AVL tree is only a cache, and therefore it
		 * might be empty.  If it is, we need to call into the
		 * backend to fetch it.
		 */
		ASSERT3U(avl_numnodes(&obj->versions), <=, 1);

		ver = avl_first(&obj->versions);
		if (ver)
			return ver;

		/* try to fetch the version from the backend */
		ver = __fetch_ver(obj, NULL);
		if (!IS_ERR(ver))
			return ver;
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
	struct objver *objver;
	struct obj *obj;
	void *cookie;

	if (!vg || !oid || !clock)
		return ERR_PTR(-EINVAL);

	objver = getver(vg, oid, clock);
	if (IS_ERR(objver))
		return objver;

	obj = objver->obj;

	if (obj->ops && obj->ops->open)
		cookie = obj->ops->open(obj->vol, oid, clock);
	else
		cookie = NULL;

	if (IS_ERR(cookie)) {
		mxunlock(&obj->lock);
		obj_putref(obj);
		return cookie;
	}

	obj->open_cookie = cookie;

	/*
	 * The object associated with 'objver' is locked and held.  We
	 * unlock it, but keep the hold on it.  Then we return the pointer
	 * to the objver to the caller.  Then, later on when the caller
	 * calls the close function, we release the object's hold.
	 *
	 * While the object version is open, we maintain this reference hold
	 * keeping the object and the version structures in memory.  Even if
	 * all links are removed, we keep the structures around and let
	 * operations use them.  This mimics the POSIX file system semantics
	 * well.
	 */

	mxunlock(&obj->lock);

	return objver;
}

int objstore_close(struct objstore *vg, void *cookie)
{
	struct objver *objver = cookie;
	struct obj *obj;
	int ret;

	if (!vg || !objver)
		return -EINVAL;

	if (vg != objver->obj->vol->vg)
		return -ENXIO;

	obj = objver->obj;

	/*
	 * Yes, we are dereferencing the pointer that the caller supplied.
	 * Yes, this is safe.  The only reason this is safe is because we
	 * rely on the caller to keep the pointer safe.  Specifically, we
	 * assume that the caller didn't transmit the pointer over the
	 * network.
	 */

	mxlock(&obj->lock);
	if (obj->ops && obj->ops->close)
		ret = obj->ops->close(obj->vol, obj->open_cookie);
	else
		ret = 0;

	if (!ret)
		obj->open_cookie = NULL;
	mxunlock(&obj->lock);

	/* release the reference obtained in objstore_open() */
	obj_putref(obj);

	return ret;
}

int objstore_getattr(struct objstore *vg, void *cookie, struct nattr *attr)
{
	struct objver *objver = cookie;
	struct obj *obj;
	int ret;

	if (!vg || !objver || !attr)
		return -EINVAL;

	if (vg != objver->obj->vol->vg)
		return -ENXIO;

	obj = objver->obj;

	mxlock(&obj->lock);
	ret = vol_getattr(obj->vol, obj->open_cookie, attr);
	mxunlock(&obj->lock);

	return ret;
}

int objstore_setattr(struct objstore *vg, void *cookie, const struct nattr *attr,
		     const unsigned valid)
{
	struct objver *objver = cookie;
	struct obj *obj;
	int ret;

	if (!vg || !objver || !attr)
		return -EINVAL;

	if (vg != objver->obj->vol->vg)
		return -ENXIO;

	obj = objver->obj;

	mxlock(&obj->lock);
	if (!valid)
		ret = 0;
	else
		ret = vol_setattr(obj->vol, obj->open_cookie, attr, valid);
	mxunlock(&obj->lock);

	return ret;
}

ssize_t objstore_read(struct objstore *vg, void *cookie, void *buf, size_t len,
		      uint64_t offset)
{
	struct objver *objver = cookie;
	struct obj *obj;
	ssize_t ret;

	if (!vg || !objver || !buf)
		return -EINVAL;

	if (len > (SIZE_MAX / 2))
		return -EOVERFLOW;

	if (vg != objver->obj->vol->vg)
		return -ENXIO;

	/* nothing to do */
	if (!len)
		return 0;

	obj = objver->obj;

	mxlock(&obj->lock);
	if (NATTR_ISDIR(objver->attrs.mode))
		/* TODO: do we need to check for other types? */
		ret = -EISDIR;
	else
		ret = vol_read(obj->vol, obj->open_cookie, buf, len, offset);
	mxunlock(&obj->lock);

	return ret;
}

ssize_t objstore_write(struct objstore *vg, void *cookie, const void *buf,
		       size_t len, uint64_t offset)
{
	struct objver *objver = cookie;
	struct obj *obj;
	ssize_t ret;

	if (!vg || !objver || !buf)
		return -EINVAL;

	if (len > (SIZE_MAX / 2))
		return -EOVERFLOW;

	if (vg != objver->obj->vol->vg)
		return -ENXIO;

	/* nothing to do */
	if (!len)
		return 0;

	obj = objver->obj;

	mxlock(&obj->lock);
	if (NATTR_ISDIR(objver->attrs.mode))
		/* TODO: do we need to check for other types? */
		ret = -EISDIR;
	else
		ret = vol_write(obj->vol, obj->open_cookie, buf, len, offset);
	mxunlock(&obj->lock);

	return ret;
}

int objstore_lookup(struct objstore *vg, void *dircookie, const char *name,
		    struct noid *child)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, '%s', %p)", __func__, vg, dircookie,
		name, child);

	if (!vg || !dirver || !name || !child)
		return -EINVAL;

	if (vg != dirver->obj->vol->vg)
		return -ENXIO;

	dir = dirver->obj;

	mxlock(&dir->lock);
	ret = vol_lookup(dir->vol, dir->open_cookie, name, child);
	mxunlock(&dir->lock);

	return ret;
}

int objstore_create(struct objstore *vg, void *dircookie, const char *name,
		    uint16_t mode, struct noid *child)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, '%s', %#o, %p)", __func__, vg,
		dircookie, name, mode, child);

	if (!vg || !dirver || !name || !child)
		return -EINVAL;

	if (vg != dirver->obj->vol->vg)
		return -ENXIO;

	dir = dirver->obj;

	mxlock(&dir->lock);
	ret = vol_create(dir->vol, dir->open_cookie, name, mode, child);
	mxunlock(&dir->lock);

	return ret;
}

int objstore_unlink(struct objstore *vg, void *dircookie, const char *name)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	cmn_err(CE_DEBUG, "%s(%p, %p, '%s')", __func__, vg, dircookie, name);

	if (!vg || !dirver || !name)
		return -EINVAL;

	if (vg != dirver->obj->vol->vg)
		return -ENXIO;

	dir = dirver->obj;

	mxlock(&dir->lock);
	ret = vol_unlink(dir->vol, dir->open_cookie, name);
	mxunlock(&dir->lock);

	return ret;
}
