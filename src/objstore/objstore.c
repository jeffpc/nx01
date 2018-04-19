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
#include <jeffpc/sexpr.h>

#include <nomad/config.h>
#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

static struct list backends;

struct mem_cache *vdev_cache;

struct backend *backend_lookup(const char *name)
{
	struct backend *backend;

	list_for_each(backend, &backends) {
		if (!strcmp(name, backend->def->name))
			return backend;
	}

	return NULL;
}

static struct backend *load_backend(const char *name)
{
	char path[FILENAME_MAX];
	struct backend *backend;

	cmn_err(CE_DEBUG, "Loading '%s' backend...", name);

	snprintf(path, sizeof(path), "libnomad_objstore_%s.so", name);

	backend = malloc(sizeof(struct backend));
	if (!backend)
		return ERR_PTR(-ENOMEM);

	backend->module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!backend->module) {
		free(backend);
		return ERR_PTR(-ENOENT);
	}

	backend->def = dlsym(backend->module, "objvdev");
	if (!backend->def) {
		dlclose(backend->module);
		free(backend);
		return ERR_PTR(-ENOENT);
	}

	return backend;
}

static int __load_backends(void)
{
	struct val *backends_sexpr;
	struct val *cur, *tmp;
	int ret;

	backends_sexpr = config_get_backends();

	ret = 0;

	sexpr_for_each(cur, tmp, backends_sexpr) {
		struct backend *backend;
		struct sym *name;

		ASSERT3U(cur->type, ==, VT_SYM);

		name = val_cast_to_sym(cur);

		backend = load_backend(sym_cstr(name));
		if (IS_ERR(backend)) {
			ret = PTR_ERR(backend);
			break;
		}

		list_insert_tail(&backends, backend);
	}

	val_putref(backends_sexpr);

	if (ret) {
		/* TODO: remove & free all backends */
	}

	return ret;
}

int objstore_init(void)
{
	int ret;

	list_create(&backends, sizeof(struct backend),
		    offsetof(struct backend, node));

	vdev_cache = mem_cache_create("vdev", sizeof(struct objstore_vdev), 0);
	if (IS_ERR(vdev_cache))
		return PTR_ERR(vdev_cache);

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

	ret = vol_init();
	if (ret)
		goto err_objver;

	ret = __load_backends();
	if (ret)
		goto err_vol;

	return 0;

err_vol:
	/* TODO: undo vol_init() */

err_objver:
	mem_cache_destroy(objver_cache);

err_obj:
	mem_cache_destroy(obj_cache);

err:
	mem_cache_destroy(vdev_cache);

	return ret;
}
