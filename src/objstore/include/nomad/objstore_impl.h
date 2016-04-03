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

#ifndef __NOMAD_OBJSTORE_IMPL_H
#define __NOMAD_OBJSTORE_IMPL_H

#include <nomad/objstore.h>

struct vol_ops {
	int (*create)(struct objstore_vol *store);
	int (*load)(struct objstore_vol *store);
	int (*getroot)(struct objstore_vol *store, struct noid *root);
};

struct obj_ops {
	int (*getversions)();

	/* open objects must be closed */
	int (*open)();		/* open an object */
	int (*close)();		/* close an object */

	/* cloned objects must be committed/aborted */
	int (*clone)();		/*
				 * create a new temp obj as a copy of
				 * existing obj
				 */
	int (*commit)();	/* make temp object live */
	int (*abort)();		/* delete temp object */

	int (*getattr)(struct objstore_vol *store, const struct nobjhndl *hndl,
		       struct nattr *attr);
	int (*setattr)();	/* set attributes of an object */
	ssize_t (*read)();	/* read portion of an object */
	ssize_t (*write)();	/* write portion of an object */

	int (*lookup)(struct objstore_vol *vol, const struct nobjhndl *dir,
	              const char *name, struct nobjhndl *child);
	int (*create)(struct objstore_vol *vol, const struct nobjhndl *dir,
	              const char *name, uint16_t mode,
	              struct nobjhndl *child);
	int (*remove)(struct objstore_vol *vol, const struct nobjhndl *dir,
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
extern int vol_getattr(struct objstore_vol *vol, const struct nobjhndl *hndl,
                       struct nattr *attr);
extern int vol_lookup(struct objstore_vol *vol, const struct nobjhndl *dir,
                      const char *name, struct nobjhndl *child);
extern int vol_create(struct objstore_vol *vol, const struct nobjhndl *dir,
                      const char *name, uint16_t mode, struct nobjhndl *child);
extern int vol_remove(struct objstore_vol *vol, const struct nobjhndl *dir,
                      const char *name);

#endif
