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

#ifndef __NOMAD_OBJSTORE_IMPL_H
#define __NOMAD_OBJSTORE_IMPL_H

#include <nomad/objstore.h>

REFCNT_INLINE_FXNS(struct objstore_vol, vol, refcnt, objstore_vol_free)

struct vol_ops {
	int (*create)(struct objstore_vol *store);
	int (*load)(struct objstore_vol *store);
	int (*getroot)(struct objstore_vol *store, struct noid *root);
};

struct obj_ops {
	int (*getversions)();

	/* open objects must be closed */
	void *(*open)(struct objstore_vol *vol, const struct noid *oid,
		      const struct nvclock *clock);
	int (*close)(struct objstore_vol *vol, void *cookie);

	/* cloned objects must be committed/aborted */
	int (*clone)();		/*
				 * create a new temp obj as a copy of
				 * existing obj
				 */
	int (*commit)();	/* make temp object live */
	int (*abort)();		/* delete temp object */

	int (*getattr)(struct objstore_vol *store, void *cookie,
		       struct nattr *attr);
	int (*setattr)();	/* set attributes of an object */
	ssize_t (*read)(struct objstore_vol *store, void *cookie,
			void *buf, size_t len, uint64_t offset);
	ssize_t (*write)(struct objstore_vol *store, void *cookie,
			 const void *buf, size_t len, uint64_t offset);

	int (*lookup)(struct objstore_vol *vol, void *dircookie,
		      const char *name, struct noid *child);
	int (*create)(struct objstore_vol *vol, void *dircookie,
		      const char *name, uint16_t mode, struct noid *child);
	int (*unlink)(struct objstore_vol *vol, void *dircookie,
		      const char *name);
};

struct objstore_vol_def {
	const char *name;
	const struct vol_ops *vol_ops;
	const struct obj_ops *obj_ops;
};

/* internal volume group management helpers */
extern int vg_init(void);
extern void vg_add_vol(struct objstore *vg, struct objstore_vol *vol);

/* wrappers for volume ops */
extern int vol_getroot(struct objstore_vol *vol, struct noid *root);

/* wrappers for object ops */
extern void *vol_open(struct objstore_vol *vol, const struct noid *oid,
		      const struct nvclock *clock);
extern int vol_close(struct objstore_vol *vol, void *cookie);
extern int vol_getattr(struct objstore_vol *vol, void *cookie,
		       struct nattr *attr);
extern ssize_t vol_read(struct objstore_vol *vol, void *cookie, void *buf,
			size_t len, uint64_t offset);
extern ssize_t vol_write(struct objstore_vol *vol, void *cookie,
			 const void *buf, size_t len, uint64_t offset);
extern int vol_lookup(struct objstore_vol *vol, void *dircookie,
		      const char *name, struct noid *child);
extern int vol_create(struct objstore_vol *vol, void *dircookie,
		      const char *name, uint16_t mode, struct noid *child);
extern int vol_unlink(struct objstore_vol *vol, void *dircookie,
		      const char *name);

#endif
