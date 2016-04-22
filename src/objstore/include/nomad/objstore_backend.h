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

#ifndef __NOMAD_OBJSTORE_BACKEND_H
#define __NOMAD_OBJSTORE_BACKEND_H

#include <nomad/objstore.h>

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
	int (*setattr)(struct objstore_vol *store, void *cookie,
		       const struct nattr *attr, const unsigned valid);
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

struct vol_ops {
	int (*create)(struct objstore_vol *store);
	int (*load)(struct objstore_vol *store);
	int (*getroot)(struct objstore_vol *store, struct noid *root);
};

struct objstore_vol_def {
	const char *name;
	const struct obj_ops *obj_ops;
	const struct vol_ops *vol_ops;
};

struct objstore_vol {
	struct objstore *vg;

	const struct objstore_vol_def *def;

	struct xuuid uuid;
	const char *path;
	enum objstore_mode mode;

	refcnt_t refcnt;

	void *private;
};

#endif
