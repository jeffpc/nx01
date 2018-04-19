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

#ifndef __NOMAD_OBJSTORE_IMPL_H
#define __NOMAD_OBJSTORE_IMPL_H

/*
 * There are three objstore include files.  Make sure to include the right
 * one.
 *
 * (1) If you are simply trying to use objstore, include <nomad/objstore.h>.
 * (2) If you are trying to write a backend, include <nomad/objstore_backend.h>.
 * (3) If you are developing objstore itself, include <nomad/objstore_impl.h>.
 */

#include <jeffpc/mem.h>
#include <jeffpc/list.h>

#include <nomad/objstore.h>
#include <nomad/objstore_backend.h>

/* backend support */
struct backend {
	struct list_node node;
	const struct objstore_vdev_def *def;
	void *module;
};

/* internal backend management */
extern struct backend *backend_lookup(const char *name);

/* internal volume management */
extern int vol_init(void);
extern void vol_add_vdev(struct objstore *vol, struct objstore_vdev *vdev);

/* internal vdev management */
extern struct mem_cache *vdev_cache;
extern void vdev_free(struct objstore_vdev *vdev);

REFCNT_INLINE_FXNS(struct objstore_vdev, vdev, refcnt, vdev_free, NULL)

/* internal object management */
extern struct mem_cache *obj_cache;
extern struct mem_cache *objver_cache;
extern struct obj *allocobj(void);
extern void freeobj(struct obj *obj);
extern struct objver *allocobjver(void);
extern void freeobjver(struct objver *ver);

REFCNT_INLINE_FXNS(struct obj, obj, refcnt, freeobj, NULL);

#endif
