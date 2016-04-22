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

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <jeffpc/error.h>

#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

/*
 * TODO: eventually turn this into something that can support multiple
 * backends.
 */
static struct backend mem_backend;
struct backend *backend;

umem_cache_t *vol_cache;

static int load_backend(struct backend *backend, const char *name)
{
	char path[FILENAME_MAX];

	snprintf(path, sizeof(path), "libnomad_objstore_%s.so", name);

	backend->module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!backend->module)
		return -ENOENT;

	backend->def = dlsym(backend->module, "objvol");
	if (!backend->def) {
		dlclose(backend->module);
		return -ENOENT;
	}

	return 0;
}

int objstore_init(void)
{
	int ret;

	vol_cache = umem_cache_create("vol", sizeof(struct objstore_vol),
				      0, NULL, NULL, NULL, NULL, NULL, 0);
	if (!vol_cache)
		return -ENOMEM;

	ret = vg_init();
	if (ret)
		goto err;

	ret = load_backend(&mem_backend, "mem");
	if (ret)
		goto err;

	backend = &mem_backend;

	return 0;

err:
	umem_cache_destroy(vol_cache);

	return ret;
}
