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

#include <stddef.h>

#include <jeffpc/error.h>
#include <jeffpc/mem.h>

#include <nomad/iter.h>
#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

static struct mem_cache *vol_cache;

static struct lock vols_lock;
static struct list vols;

int vol_init(void)
{
	vol_cache = mem_cache_create("vol", sizeof(struct objstore), 0);
	if (IS_ERR(vol_cache))
		return PTR_ERR(vol_cache);

	MXINIT(&vols_lock);

	list_create(&vols, sizeof(struct objstore),
		    offsetof(struct objstore, node));

	return 0;
}

void vol_fini(void)
{
	list_destroy(&vols);

	MXDESTROY(&vols_lock);

	mem_cache_destroy(vol_cache);
}

static int objcmp(const void *va, const void *vb)
{
	const struct obj *a = va;
	const struct obj *b = vb;

	return noid_cmp(&a->oid, &b->oid);
}

struct objstore *objstore_vol_create(struct objstore_vdev *vdev,
				     const char *name)
{
	struct objstore *vol;
	int ret;

	if (!vdev->def->create_vol)
		return ERR_PTR(-ENOTSUP);

	vol = mem_cache_alloc(vol_cache);
	if (!vol)
		return ERR_PTR(-ENOMEM);

	vol->name = strdup(name);
	if (!vol->name) {
		mem_cache_free(vol_cache, vol);
		return ERR_PTR(-ENOMEM);
	}

	vol->ops = NULL;
	vol->vdev = vdev_getref(vdev);
	vol->private = NULL;

	MXINIT(&vol->lock);
	avl_create(&vol->objs, objcmp, sizeof(struct obj),
		   offsetof(struct obj, node));

	xuuid_generate(&vol->id);

	ret = vdev->def->create_vol(vol);
	if (ret)
		goto err;

	MXLOCK(&vols_lock);
	list_insert_tail(&vols, vol_getref(vol));
	MXUNLOCK(&vols_lock);

	return vol;

err:
	vdev_putref(vol->vdev);

	free((char *) vol->name);

	mem_cache_free(vol_cache, vol);

	return ERR_PTR(ret);
}

struct objstore *objstore_vol_lookup(const struct xuuid *volid)
{
	struct objstore *vol;

	MXLOCK(&vols_lock);
	list_for_each(vol, &vols) {
		if (!xuuid_compare(volid, &vol->id))
			break;
	}

	vol = vol_getref(vol);
	MXUNLOCK(&vols_lock);

	return vol ? vol : ERR_PTR(-ENOENT);
}

void objstore_vol_free(struct objstore *vol)
{
	ASSERT0(avl_numnodes(&vol->objs));
	avl_destroy(&vol->objs);

	vdev_putref(vol->vdev);

	free((char *) vol->name);
	mem_cache_free(vol_cache, vol);
}

/*
 * Find the object with oid, if there isn't one, allocate one and add it to
 * the vol's list of objects.
 *
 * Returns obj (locked and referenced) on success, negated errno on failure.
 */
static struct obj *__find_or_alloc(struct objstore *vol, const struct noid *oid)
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
		MXLOCK(&vol->lock);

		/* try to find the object */
		obj = obj_getref(avl_find(&vol->objs, &key, &where));

		/* not found and this is the second attempt -> insert it */
		if (!obj && newobj) {
			avl_insert(&vol->objs, obj_getref(newobj), where);
			obj = newobj;
			newobj = NULL;
			inserted = true;
		}

		MXUNLOCK(&vol->lock);

		/* found or inserted -> we're done */
		if (obj) {
			/* newly inserted objects are already locked */
			if (!inserted)
				MXLOCK(&obj->lock);

			if (obj->state == OBJ_STATE_DEAD) {
				/* this is a dead one, try again */
				MXUNLOCK(&obj->lock);
				obj_putref(obj);
				continue;
			}

			if (newobj) {
				MXUNLOCK(&newobj->lock);
				obj_putref(newobj);
			}

			break;
		}

		/* allocate a new object */
		newobj = allocobj();
		if (!newobj)
			return ERR_PTR(-ENOMEM);

		MXLOCK(&newobj->lock);

		newobj->oid = *oid;
		newobj->vol = vol_getref(vol);

		/* retry the search, and insert if necessary */
	}

	return obj;
}

/*
 * Given a vol and an oid, find the corresponding object structure.
 *
 * Return with the object locked and referenced.
 */
static struct obj *getobj(struct objstore *vol, const struct noid *oid)
{
	struct obj *obj;
	int ret;

	obj = __find_or_alloc(vol, oid);
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
				MXLOCK(&vol->lock);
				avl_remove(&vol->objs, obj);
				MXUNLOCK(&vol->lock);
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

	return obj;

err_obj:
	MXUNLOCK(&obj->lock);
	obj_putref(obj);

err:
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
 * Given a vol, an oid, and a vector clock, find the corresponding object
 * version structure.
 *
 * Return the found version, with the object referenced and locked.
 */
static struct objver *getver(struct objstore *vol, const struct noid *oid,
			     const struct nvclock *clock)
{
	struct objver *ver;
	struct obj *obj;

	/*
	 * First, find the object based on the oid.
	 */
	obj = getobj(vol, oid);
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

	MXUNLOCK(&obj->lock);
	obj_putref(obj);
	return ver;
}

int objstore_getroot(struct objstore *vol, struct noid *root)
{
	int ret;

	if (!vol || !root)
		return -EINVAL;

	if (vol->ops && vol->ops->getroot)
		ret = vol->ops->getroot(vol, root);
	else
		ret = -ENOTSUP;

	return ret;
}

void *objstore_open(struct objstore *vol, const struct noid *oid,
		    const struct nvclock *clock)
{
	struct objver *objver;
	struct obj *obj;
	int ret;

	if (!vol || !oid || !clock)
		return ERR_PTR(-EINVAL);

	objver = getver(vol, oid, clock);
	if (IS_ERR(objver))
		return objver;

	obj = objver->obj;

	if (!objver->open_count) {
		/* first open */
		if (obj->ops && obj->ops->open)
			ret = obj->ops->open(objver);
		else
			ret = 0;

		if (ret) {
			MXUNLOCK(&obj->lock);
			obj_putref(obj);
			return ERR_PTR(ret);
		}
	}

	objver->open_count++;

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

	MXUNLOCK(&obj->lock);

	return objver;
}

int objstore_close(struct objstore *vol, void *cookie)
{
	struct objver *objver = cookie;
	struct obj *obj;
	bool putref;
	int ret;

	if (!vol || !objver)
		return -EINVAL;

	if (vol != objver->obj->vol)
		return -ENXIO;

	obj = objver->obj;

	/*
	 * Yes, we are dereferencing the pointer that the caller supplied.
	 * Yes, this is safe.  The only reason this is safe is because we
	 * rely on the caller to keep the pointer safe.  Specifically, we
	 * assume that the caller didn't transmit the pointer over the
	 * network.
	 */

	MXLOCK(&obj->lock);
	objver->open_count--;
	putref = true;

	if (objver->open_count) {
		ret = 0;
		putref = false;
	} else if (obj->ops && obj->ops->close) {
		/* last close */
		ret = obj->ops->close(objver);
	} else {
		ret = 0;
	}

	if (ret)
		objver->open_count++; /* undo earlier decrement */
	MXUNLOCK(&obj->lock);

	/* release the reference obtained in objstore_open() */
	if (putref && !ret)
		obj_putref(obj);

	return ret;
}

int objstore_getattr(struct objstore *vol, void *cookie, struct nattr *attr)
{
	struct objver *objver = cookie;
	struct obj *obj;
	int ret;

	if (!vol || !objver || !attr)
		return -EINVAL;

	if (vol != objver->obj->vol)
		return -ENXIO;

	obj = objver->obj;

	if (!obj->ops || !obj->ops->getattr)
		return -ENOTSUP;

	MXLOCK(&obj->lock);
	ret = obj->ops->getattr(objver, attr);
	MXUNLOCK(&obj->lock);

	return ret;
}

int objstore_setattr(struct objstore *vol, void *cookie, struct nattr *attr,
		     const unsigned valid)
{
	struct objver *objver = cookie;
	struct obj *obj;
	int ret;

	if (!vol || !objver || !attr)
		return -EINVAL;

	if (vol != objver->obj->vol)
		return -ENXIO;

	obj = objver->obj;

	if (!obj->ops || !obj->ops->setattr)
		return -ENOTSUP;

	MXLOCK(&obj->lock);
	ret = obj->ops->setattr(objver, attr, valid);
	MXUNLOCK(&obj->lock);

	return ret;
}

ssize_t objstore_read(struct objstore *vol, void *cookie, void *buf, size_t len,
		      uint64_t offset)
{
	struct objver *objver = cookie;
	struct obj *obj;
	ssize_t ret;

	if (!vol || !objver || !buf)
		return -EINVAL;

	if (len > (SIZE_MAX / 2))
		return -EOVERFLOW;

	if (vol != objver->obj->vol)
		return -ENXIO;

	obj = objver->obj;

	if (!obj->ops || !obj->ops->read)
		return -ENOTSUP;

	/* nothing to do */
	if (!len)
		return 0;

	MXLOCK(&obj->lock);
	if (NATTR_ISDIR(objver->attrs.mode))
		/* TODO: do we need to check for other types? */
		ret = -EISDIR;
	else
		ret = obj->ops->read(objver, buf, len, offset);
	MXUNLOCK(&obj->lock);

	return ret;
}

ssize_t objstore_write(struct objstore *vol, void *cookie, const void *buf,
		       size_t len, uint64_t offset)
{
	struct objver *objver = cookie;
	struct obj *obj;
	ssize_t ret;

	if (!vol || !objver || !buf)
		return -EINVAL;

	if (len > (SIZE_MAX / 2))
		return -EOVERFLOW;

	if (vol != objver->obj->vol)
		return -ENXIO;

	obj = objver->obj;

	if (!obj->ops || !obj->ops->write)
		return -ENOTSUP;

	/* nothing to do */
	if (!len)
		return 0;

	MXLOCK(&obj->lock);
	if (NATTR_ISDIR(objver->attrs.mode))
		/* TODO: do we need to check for other types? */
		ret = -EISDIR;
	else
		ret = obj->ops->write(objver, buf, len, offset);
	MXUNLOCK(&obj->lock);

	return ret;
}

int objstore_lookup(struct objstore *vol, void *dircookie, const char *name,
		    struct noid *child)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	if (!vol || !dirver || !name || !child)
		return -EINVAL;

	if (vol != dirver->obj->vol)
		return -ENXIO;

	dir = dirver->obj;

	if (!dir->ops || !dir->ops->lookup)
		return -ENOTSUP;

	MXLOCK(&dir->lock);
	if (!NATTR_ISDIR(dirver->attrs.mode))
		ret = -ENOTDIR;
	else
		ret = dir->ops->lookup(dirver, name, child);
	MXUNLOCK(&dir->lock);

	return ret;
}

int objstore_create(struct objstore *vol, void *dircookie, const char *name,
		    uint16_t mode, struct noid *child)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	if (!vol || !dirver || !name || !child)
		return -EINVAL;

	if (vol != dirver->obj->vol)
		return -ENXIO;

	dir = dirver->obj;

	if (!dir->ops || !dir->ops->create)
		return -ENOTSUP;

	MXLOCK(&dir->lock);
	if (!NATTR_ISDIR(dirver->attrs.mode))
		ret = -ENOTDIR;
	else
		ret = dir->ops->create(dirver, name, mode, child);
	MXUNLOCK(&dir->lock);

	return ret;
}

static struct obj *getobj_in_dir(struct objver *dirver, const char *name)
{
	struct noid child_oid;
	int ret;

	ret = dirver->obj->ops->lookup(dirver, name, &child_oid);
	if (ret)
		return ERR_PTR(ret);

	return getobj(dirver->obj->vol, &child_oid);
}

int objstore_unlink(struct objstore *vol, void *dircookie, const char *name)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	if (!vol || !dirver || !name)
		return -EINVAL;

	if (vol != dirver->obj->vol)
		return -ENXIO;

	dir = dirver->obj;

	if (!dir->ops || !dir->ops->unlink || !dir->ops->lookup)
		return -ENOTSUP;

	MXLOCK(&dir->lock);
	if (!NATTR_ISDIR(dirver->attrs.mode)) {
		ret = -ENOTDIR;
	} else {
		struct obj *child;

		child = getobj_in_dir(dirver, name);
		if (!IS_ERR(child)) {
			ret = dir->ops->unlink(dirver, name, child);

			MXUNLOCK(&child->lock);
			obj_putref(child);
		} else {
			ret = PTR_ERR(child);
		}
	}
	MXUNLOCK(&dir->lock);

	return ret;
}

int objstore_getdent(struct objstore *vol, void *dircookie,
		     const uint64_t offset, struct noid *child,
		     char **childname, uint64_t *entry_size)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	if (!vol || !dirver)
		return -EINVAL;

	if (vol != dirver->obj->vol)
		return -ENXIO;

	dir = dirver->obj;

	if (!dir->ops || !dir->ops->getdent)
		return -ENOTSUP;

	MXLOCK(&dir->lock);
	if (!NATTR_ISDIR(dirver->attrs.mode))
		ret = -ENOTDIR;
	else
		ret = dir->ops->getdent(dirver, offset, child, childname,
					entry_size);
	MXUNLOCK(&dir->lock);

	return ret;
}
