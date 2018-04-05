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

static struct mem_cache *pool_cache;

static struct lock pools_lock;
static struct list pools;

int pool_init(void)
{
	struct objstore *filecache;

	pool_cache = mem_cache_create("pool", sizeof(struct objstore), 0);
	if (IS_ERR(pool_cache))
		return PTR_ERR(pool_cache);

	mxinit(&pools_lock);

	list_create(&pools, sizeof(struct objstore),
		    offsetof(struct objstore, node));

	filecache = objstore_pool_create("file$");
	if (IS_ERR(filecache)) {
		mem_cache_destroy(pool_cache);
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

struct objstore *objstore_pool_create(const char *name)
{
	struct objstore *pool;

	pool = mem_cache_alloc(pool_cache);
	if (!pool)
		return ERR_PTR(-ENOMEM);

	pool->name = strdup(name);
	if (!pool->name) {
		mem_cache_free(pool_cache, pool);
		return ERR_PTR(-ENOMEM);
	}

	pool->vol = NULL;

	mxinit(&pool->lock);
	avl_create(&pool->objs, objcmp, sizeof(struct obj),
		   offsetof(struct obj, node));

	mxlock(&pools_lock);
	list_insert_tail(&pools, pool);
	mxunlock(&pools_lock);

	return pool;
}

void pool_add_vol(struct objstore *pool, struct objstore_vol *vol)
{
	mxlock(&pool->lock);
	ASSERT3P(pool->vol, ==, NULL);
	pool->vol = vol;
	mxunlock(&pool->lock);
}

struct objstore *objstore_pool_lookup(const char *name)
{
	struct objstore *pool;

	mxlock(&pools_lock);
	list_for_each(pool, &pools)
		if (!strcmp(name, pool->name))
			break;
	mxunlock(&pools_lock);

	return pool;
}

static struct objstore_vol *findvol(struct objstore *pool)
{
	struct objstore_vol *vol;

	mxlock(&pool->lock);
	vol = vol_getref(pool->vol);
	mxunlock(&pool->lock);

	return vol;
}

/*
 * Find the object with oid, if there isn't one, allocate one and add it to
 * the pool's list of objects.
 *
 * Returns obj (locked and referenced) on success, negated errno on failure.
 */
static struct obj *__find_or_alloc(struct objstore *pool, struct
				   objstore_vol *vol, const struct noid *oid)
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
		mxlock(&pool->lock);

		/* try to find the object */
		obj = obj_getref(avl_find(&pool->objs, &key, &where));

		/* not found and this is the second attempt -> insert it */
		if (!obj && newobj) {
			avl_insert(&pool->objs, obj_getref(newobj), where);
			obj = newobj;
			newobj = NULL;
			inserted = true;
		}

		mxunlock(&pool->lock);

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
 * Given a pool and an oid, find the corresponding object structure.
 *
 * Return with the object locked and referenced.
 */
static struct obj *getobj(struct objstore *pool, const struct noid *oid)
{
	struct objstore_vol *vol;
	struct obj *obj;
	int ret;

	vol = findvol(pool);
	if (!vol)
		return ERR_PTR(-ENXIO);

	obj = __find_or_alloc(pool, vol, oid);
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
				mxlock(&pool->lock);
				avl_remove(&pool->objs, obj);
				mxunlock(&pool->lock);
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
 * Given a pool, an oid, and a vector clock, find the corresponding object
 * version structure.
 *
 * Return the found version, with the object referenced and locked.
 */
static struct objver *getver(struct objstore *pool, const struct noid *oid,
			     const struct nvclock *clock)
{
	struct objver *ver;
	struct obj *obj;

	/*
	 * First, find the object based on the oid.
	 */
	obj = getobj(pool, oid);
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

int objstore_getroot(struct objstore *pool, struct noid *root)
{
	struct objstore_vol *vol;
	int ret;

	if (!pool || !root)
		return -EINVAL;

	vol = findvol(pool);
	if (!vol)
		return -ENXIO;

	if (vol->ops && vol->ops->getroot)
		ret = vol->ops->getroot(vol, root);
	else
		ret = -ENOTSUP;

	vol_putref(vol);

	return ret;
}

void *objstore_open(struct objstore *pool, const struct noid *oid,
		    const struct nvclock *clock)
{
	struct objver *objver;
	struct obj *obj;
	int ret;

	if (!pool || !oid || !clock)
		return ERR_PTR(-EINVAL);

	objver = getver(pool, oid, clock);
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
			mxunlock(&obj->lock);
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

	mxunlock(&obj->lock);

	return objver;
}

int objstore_close(struct objstore *pool, void *cookie)
{
	struct objver *objver = cookie;
	struct obj *obj;
	bool putref;
	int ret;

	if (!pool || !objver)
		return -EINVAL;

	if (pool != objver->obj->vol->pool)
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
	mxunlock(&obj->lock);

	/* release the reference obtained in objstore_open() */
	if (putref && !ret)
		obj_putref(obj);

	return ret;
}

int objstore_getattr(struct objstore *pool, void *cookie, struct nattr *attr)
{
	struct objver *objver = cookie;
	struct obj *obj;
	int ret;

	if (!pool || !objver || !attr)
		return -EINVAL;

	if (pool != objver->obj->vol->pool)
		return -ENXIO;

	obj = objver->obj;

	if (!obj->ops || !obj->ops->getattr)
		return -ENOTSUP;

	mxlock(&obj->lock);
	ret = obj->ops->getattr(objver, attr);
	mxunlock(&obj->lock);

	return ret;
}

int objstore_setattr(struct objstore *pool, void *cookie, struct nattr *attr,
		     const unsigned valid)
{
	struct objver *objver = cookie;
	struct obj *obj;
	int ret;

	if (!pool || !objver || !attr)
		return -EINVAL;

	if (pool != objver->obj->vol->pool)
		return -ENXIO;

	obj = objver->obj;

	if (!obj->ops || !obj->ops->setattr)
		return -ENOTSUP;

	mxlock(&obj->lock);
	ret = obj->ops->setattr(objver, attr, valid);
	mxunlock(&obj->lock);

	return ret;
}

ssize_t objstore_read(struct objstore *pool, void *cookie, void *buf, size_t len,
		      uint64_t offset)
{
	struct objver *objver = cookie;
	struct obj *obj;
	ssize_t ret;

	if (!pool || !objver || !buf)
		return -EINVAL;

	if (len > (SIZE_MAX / 2))
		return -EOVERFLOW;

	if (pool != objver->obj->vol->pool)
		return -ENXIO;

	obj = objver->obj;

	if (!obj->ops || !obj->ops->read)
		return -ENOTSUP;

	/* nothing to do */
	if (!len)
		return 0;

	mxlock(&obj->lock);
	if (NATTR_ISDIR(objver->attrs.mode))
		/* TODO: do we need to check for other types? */
		ret = -EISDIR;
	else
		ret = obj->ops->read(objver, buf, len, offset);
	mxunlock(&obj->lock);

	return ret;
}

ssize_t objstore_write(struct objstore *pool, void *cookie, const void *buf,
		       size_t len, uint64_t offset)
{
	struct objver *objver = cookie;
	struct obj *obj;
	ssize_t ret;

	if (!pool || !objver || !buf)
		return -EINVAL;

	if (len > (SIZE_MAX / 2))
		return -EOVERFLOW;

	if (pool != objver->obj->vol->pool)
		return -ENXIO;

	obj = objver->obj;

	if (!obj->ops || !obj->ops->write)
		return -ENOTSUP;

	/* nothing to do */
	if (!len)
		return 0;

	mxlock(&obj->lock);
	if (NATTR_ISDIR(objver->attrs.mode))
		/* TODO: do we need to check for other types? */
		ret = -EISDIR;
	else
		ret = obj->ops->write(objver, buf, len, offset);
	mxunlock(&obj->lock);

	return ret;
}

int objstore_lookup(struct objstore *pool, void *dircookie, const char *name,
		    struct noid *child)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	if (!pool || !dirver || !name || !child)
		return -EINVAL;

	if (pool != dirver->obj->vol->pool)
		return -ENXIO;

	dir = dirver->obj;

	if (!dir->ops || !dir->ops->lookup)
		return -ENOTSUP;

	mxlock(&dir->lock);
	if (!NATTR_ISDIR(dirver->attrs.mode))
		ret = -ENOTDIR;
	else
		ret = dir->ops->lookup(dirver, name, child);
	mxunlock(&dir->lock);

	return ret;
}

int objstore_create(struct objstore *pool, void *dircookie, const char *name,
		    uint16_t mode, struct noid *child)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	if (!pool || !dirver || !name || !child)
		return -EINVAL;

	if (pool != dirver->obj->vol->pool)
		return -ENXIO;

	dir = dirver->obj;

	if (!dir->ops || !dir->ops->create)
		return -ENOTSUP;

	mxlock(&dir->lock);
	if (!NATTR_ISDIR(dirver->attrs.mode))
		ret = -ENOTDIR;
	else
		ret = dir->ops->create(dirver, name, mode, child);
	mxunlock(&dir->lock);

	return ret;
}

static struct obj *getobj_in_dir(struct objver *dirver, const char *name)
{
	struct noid child_oid;
	int ret;

	ret = dirver->obj->ops->lookup(dirver, name, &child_oid);
	if (ret)
		return ERR_PTR(ret);

	return getobj(dirver->obj->vol->pool, &child_oid);
}

int objstore_unlink(struct objstore *pool, void *dircookie, const char *name)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	if (!pool || !dirver || !name)
		return -EINVAL;

	if (pool != dirver->obj->vol->pool)
		return -ENXIO;

	dir = dirver->obj;

	if (!dir->ops || !dir->ops->unlink || !dir->ops->lookup)
		return -ENOTSUP;

	mxlock(&dir->lock);
	if (!NATTR_ISDIR(dirver->attrs.mode)) {
		ret = -ENOTDIR;
	} else {
		struct obj *child;

		child = getobj_in_dir(dirver, name);
		if (!IS_ERR(child)) {
			ret = dir->ops->unlink(dirver, name, child);

			mxunlock(&child->lock);
			obj_putref(child);
		} else {
			ret = PTR_ERR(child);
		}
	}
	mxunlock(&dir->lock);

	return ret;
}

int objstore_getdent(struct objstore *pool, void *dircookie,
		     const uint64_t offset, struct noid *child,
		     char **childname, uint64_t *entry_size)
{
	struct objver *dirver = dircookie;
	struct obj *dir;
	int ret;

	if (!pool || !dirver)
		return -EINVAL;

	if (pool != dirver->obj->vol->pool)
		return -ENXIO;

	dir = dirver->obj;

	if (!dir->ops || !dir->ops->getdent)
		return -ENOTSUP;

	mxlock(&dir->lock);
	if (!NATTR_ISDIR(dirver->attrs.mode))
		ret = -ENOTDIR;
	else
		ret = dir->ops->getdent(dirver, offset, child, childname,
					entry_size);
	mxunlock(&dir->lock);

	return ret;
}
