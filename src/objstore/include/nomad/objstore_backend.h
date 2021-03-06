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

#ifndef __NOMAD_OBJSTORE_BACKEND_H
#define __NOMAD_OBJSTORE_BACKEND_H

#include <sys/avl.h>

#include <nomad/objstore.h>

enum obj_state {
	OBJ_STATE_NEW = 0,	/* newly allocated */
	OBJ_STATE_LIVE,		/* fully initialized */
	OBJ_STATE_DEAD,		/* initialization failed */
};

struct obj {
	/* key */
	struct noid oid;

	/* value */
	avl_tree_t versions;	/* cached versions */
	uint64_t nversions;	/* number of versions */
	uint32_t nlink;		/* file link count */
	void *private;

	/* misc */
	enum obj_state state;
	refcnt_t refcnt;
	struct lock lock;
	avl_node_t node;

	/* constant for the lifetime of the object */
	struct objstore *vol;
	const struct obj_ops *ops;
};

struct objver {
	/* key */
	struct nvclock *clock;

	/* value */
	/*
	 * Everything in the attrs structure is used for attribute storage
	 * except for:
	 *   - nlink: use nlink field in struct obj
	 */
	struct nattr attrs;
	void *private;
	uint32_t open_count;

	/* misc */
	struct obj *obj;
	avl_node_t node;
};

struct obj_ops {
	int (*getversion)(struct objver *ver);

	/* open objects must be closed */
	int (*open)(struct objver *ver);
	int (*close)(struct objver *ver);

	/* cloned objects must be committed/aborted */
	int (*clone)();		/*
				 * create a new temp obj as a copy of
				 * existing obj
				 */
	int (*commit)();	/* make temp object live */
	int (*abort)();		/* delete temp object */

	int (*getattr)(struct objver *ver, struct nattr *attr);
	int (*setattr)(struct objver *ver, struct nattr *attr,
		       const unsigned valid);
	ssize_t (*read)(struct objver *ver, void *buf, size_t len,
			uint64_t offset);
	ssize_t (*write)(struct objver *ver, const void *buf, size_t len,
			 uint64_t offset);

	int (*lookup)(struct objver *dirver, const char *name,
		      struct noid *child);
	int (*create)(struct objver *dirver, const char *name,
		      uint16_t mode, struct noid *child);
	int (*unlink)(struct objver *dirver, const char *name,
		      struct obj *child);

	int (*getdent)(struct objver *dirver, const uint64_t offset,
		       struct noid *child, char **childname,
		       uint64_t *entry_size);

	/*
	 * Called just before the generic object is freed.
	 */
	void (*free)(struct obj *obj);
};

struct vol_ops {
	int (*getroot)(struct objstore *vol, struct noid *root);
	int (*allocobj)(struct obj *obj);
};

struct objstore_vdev_def {
	const char *name;

	int (*create)(struct objstore_vdev *vdev);
	int (*create_vol)(struct objstore *vol);
	int (*load)(struct objstore_vdev *vdev);
};

#endif
