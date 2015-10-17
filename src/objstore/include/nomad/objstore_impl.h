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
	int (*getroot)(struct objstore_vol *store, struct nobjhndl *hndl);
};

struct obj_ops {
	int (*getversions)();

	/* open objects must be closed */
	int (*open)();		/* open an object */
	int (*close)();		/* close an object */

	/* created/cloned objects must be committed/aborted */
	int (*create)();	/* create a new temp object */
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
};

struct objstore_vol_def {
	const char *name;
	const struct vol_ops *vol_ops;
	const struct obj_ops *obj_ops;
};

#endif
