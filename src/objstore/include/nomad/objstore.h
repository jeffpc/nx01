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

#ifndef __NOMAD_OBJSTORE_H
#define __NOMAD_OBJSTORE_H

#include <sys/avl.h>

#include <jeffpc/refcnt.h>
#include <jeffpc/synch.h>
#include <jeffpc/list.h>

#include <nomad/types.h>

enum {
	OBJ_ATTR_MODE	= 0x01,
	OBJ_ATTR_SIZE	= 0x02,
};

struct objstore_vdev {
	const struct objstore_vdev_def *def;

	struct xuuid uuid;
	const char *path;

	refcnt_t refcnt;

	struct list_node node;

	void *private;
};

struct objstore {
	struct list_node node;

	struct lock lock;

	avl_tree_t objs;

	/* the following can be read without locking the volume */
	refcnt_t refcnt;
	const char *name;
	const struct vol_ops *ops;
	struct objstore_vdev *vdev;
	struct xuuid id;

	void *private;
};

extern int objstore_init(void);

/* vdev management */
extern struct objstore_vdev *objstore_vdev_create(const char *type,
						  const char *path);
extern struct objstore_vdev *objstore_vdev_load(const char *type,
						const char *path);
extern void objstore_vdev_free(struct objstore_vdev *vdev);

REFCNT_INLINE_FXNS(struct objstore_vdev, vdev, refcnt, objstore_vdev_free, NULL)

/* volume management */
extern struct objstore *objstore_vol_create(struct objstore_vdev *vdev,
					    const char *name);
extern struct objstore *objstore_vol_lookup(const struct xuuid *volid);
extern void objstore_vol_free(struct objstore *vol);

REFCNT_INLINE_FXNS(struct objstore, vol, refcnt, objstore_vol_free, NULL)

/* volume operations */
extern int objstore_getroot(struct objstore *vol, struct noid *root);

/* object operations */
extern void *objstore_open(struct objstore *vol, const struct noid *oid,
			   const struct nvclock *clock);
extern int objstore_close(struct objstore *vol, void *cookie);
extern int objstore_getattr(struct objstore *vol, void *cookie,
			    struct nattr *attr);
extern int objstore_setattr(struct objstore *vol, void *cookie,
			    struct nattr *attr, const unsigned valid);
extern ssize_t objstore_read(struct objstore *vol, void *cookie, void *buf,
			     size_t len, uint64_t offset);
extern ssize_t objstore_write(struct objstore *vol, void *cookie,
			      const void *buf, size_t len, uint64_t offset);
extern int objstore_lookup(struct objstore *vol, void *dircookie,
			   const char *name, struct noid *child);
extern int objstore_create(struct objstore *vol, void *dircookie,
			   const char *name, uint16_t mode, struct noid *child);
extern int objstore_unlink(struct objstore *vol, void *dircookie,
			   const char *name);
extern int objstore_getdent(struct objstore *vol, void *dircookie,
			    const uint64_t offset, struct noid *child,
			    char **childname, uint64_t *entry_size);

#endif
