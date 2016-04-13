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

#ifndef __NOMAD_OBJSTORE_H
#define __NOMAD_OBJSTORE_H

#include <sys/list.h>

#include <nomad/types.h>
#include <nomad/mutex.h>

enum objstore_mode {
	OS_MODE_CACHE,
	OS_MODE_STORE,
};

struct objstore_vol;

struct objstore {
	list_node_t node;

	pthread_mutex_t lock;
	const char *name;
	struct objstore_vol *vol;
};

struct objstore_vol_def;

struct objstore_vol {
	struct objstore *vg;

	const struct objstore_vol_def *def;

	struct xuuid uuid;
	const char *path;
	enum objstore_mode mode;

	void *private;
};

extern int objstore_init(void);

/* volume group management */
extern struct objstore *objstore_vg_create(const char *name);
extern struct objstore *objstore_vg_lookup(const char *name);

/* volume management */
extern int objstore_vol_create(struct objstore *vg, const char *path,
			       enum objstore_mode mode);
extern int objstore_vol_load(struct objstore *vg, struct xuuid *uuid,
			     const char *path);

/* volume operations */
extern int objstore_getroot(struct objstore *vg, struct noid *root);

/* object operations */
extern int objstore_getattr(struct objstore *vg, const struct noid *oid,
			    const struct nvclock *clock, struct nattr *attr);
extern int objstore_lookup(struct objstore *vg, const struct noid *dir_oid,
			   const struct nvclock *dir_clock, const char *name,
			   struct noid *child);
extern int objstore_create(struct objstore *vg, const struct noid *doid,
			   const struct nvclock *dclock, const char *name,
			   uint16_t mode, struct noid *child);
extern int objstore_remove(struct objstore *vg, const struct noid *doid,
			   const struct nvclock *dclock, const char *name);

#endif
