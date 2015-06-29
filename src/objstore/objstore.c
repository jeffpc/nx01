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

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nomad/error.h>
#include <nomad/objstore.h>
#include <nomad/objstore_impl.h>

struct backend {
	const struct objstore_def *def;
	void *module;
};

/*
 * TODO: eventually turn this into something that can support multiple
 * backends.
 */
static struct backend mem_backend;
static struct backend *backend;

static int load_backend(struct backend *backend, const char *name)
{
	char path[FILENAME_MAX];

	snprintf(path, sizeof(path), "libnomad_objstore_%s.so", name);

	backend->module = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!backend->module)
		return ENOENT;

	backend->def = dlsym(backend->module, "objstore");
	if (!backend->def) {
		dlclose(backend->module);
		return ENOENT;
	}

	return 0;
}

int objstore_init(void)
{
	int ret;

	ret = load_backend(&mem_backend, "mem");
	if (ret)
		return ret;

	backend = &mem_backend;

	return 0;
}

struct objstore *objstore_store_create(const char *path, enum objstore_mode mode)
{
	struct objstore *s;
	int ret;

	if (!backend->def->store_ops->create)
		return ERR_PTR(ENOTSUP);

	s = malloc(sizeof(struct objstore));
	if (!s)
		return ERR_PTR(ENOMEM);

	s->def = backend->def;
	s->mode = mode;
	s->path = strdup(path);
	if (!s->path) {
		ret = ENOMEM;
		goto err;
	}

	ret = s->def->store_ops->create(s);
	if (ret)
		goto err_path;

	return s;

err_path:
	free((char *) s->path);

err:
	free(s);

	return ERR_PTR(ret);
}

struct objstore *objstore_store_load(struct nuuid *uuid, const char *path)
{
	return ERR_PTR(ENOTSUP);
}
