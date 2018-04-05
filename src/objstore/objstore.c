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

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <jeffpc/error.h>

#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

static struct backend mem_backend;
static struct backend posix_backend;

struct mem_cache *vol_cache;

struct backend *backend_lookup(const char *name)
{
	if (!strcmp(name, "mem"))
		return &mem_backend;
	if (!strcmp(name, "posix"))
		return &posix_backend;
	return NULL;
}

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

	vol_cache = mem_cache_create("vol", sizeof(struct objstore_vol), 0);
	if (IS_ERR(vol_cache))
		return PTR_ERR(vol_cache);

	obj_cache = mem_cache_create("obj", sizeof(struct obj), 0);
	if (IS_ERR(obj_cache)) {
		ret = PTR_ERR(obj_cache);
		goto err;
	}

	objver_cache = mem_cache_create("objver", sizeof(struct objver), 0);
	if (IS_ERR(objver_cache)) {
		ret = PTR_ERR(objver_cache);
		goto err_obj;
	}

	ret = pool_init();
	if (ret)
		goto err_objver;

	ret = load_backend(&mem_backend, "mem");
	if (ret)
		goto err_objver;

	ret = load_backend(&posix_backend, "posix");
	if (ret)
		goto err_objver;

	return 0;

err_objver:
	mem_cache_destroy(objver_cache);

err_obj:
	mem_cache_destroy(obj_cache);

err:
	mem_cache_destroy(vol_cache);

	return ret;
}
