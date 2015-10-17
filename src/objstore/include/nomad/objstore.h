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

#ifndef __NOMAD_OBJSTORE_H
#define __NOMAD_OBJSTORE_H

#include <nomad/types.h>

enum objstore_mode {
	OS_MODE_CACHE,
	OS_MODE_STORE,
};

struct objstore_def;

struct objstore {
	const struct objstore_def *def;

	struct nuuid uuid;
	const char *path;
	enum objstore_mode mode;

	void *private;
};

extern int objstore_init(void);
extern struct objstore *objstore_store_create(const char *path,
					      enum objstore_mode mode);
extern struct objstore *objstore_store_load(struct nuuid *uuid, const char *path);

/* store operations */
extern int objstore_getroot(struct objstore *store, struct nobjhndl *hndl);

/* object operations */
extern int objstore_obj_getattr(struct objstore *store,
				const struct nobjhndl *hndl,
				struct nattr *attr);

#endif
