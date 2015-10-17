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

#include <stddef.h>

#include <nomad/error.h>
#include <nomad/mutex.h>
#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

static pthread_mutex_t vgs_lock;
static list_t vgs;

int objstore_vg_init(void)
{
	struct objstore *filecache;

	mxinit(&vgs_lock);

	list_create(&vgs, sizeof(struct objstore),
		    offsetof(struct objstore, node));

	filecache = objstore_vg_create("file$");
	if (IS_ERR(filecache))
		return PTR_ERR(filecache);

	return 0;
}

struct objstore *objstore_vg_create(const char *name)
{
	struct objstore *vg;

	vg = malloc(sizeof(struct objstore));
	if (!vg)
		return ERR_PTR(ENOMEM);

	vg->name = strdup(name);
	if (!vg->name) {
		free(vg);
		return ERR_PTR(ENOMEM);
	}

	list_create(&vg->vols, sizeof(struct objstore_vol),
		    offsetof(struct objstore_vol, vg_list));

	mxlock(&vgs_lock);
	list_insert_tail(&vgs, vg);
	mxunlock(&vgs_lock);

	return vg;
}
