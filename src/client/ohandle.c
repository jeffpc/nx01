/*
 * Copyright (c) 2016-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/rand.h>
#include <jeffpc/mem.h>

#include "ohandle.h"

static struct mem_cache *ohandle_cache;

int ohandle_init(void)
{
	ohandle_cache = mem_cache_create("ohandle", sizeof(struct ohandle), 0);
	return IS_ERR(ohandle_cache) ? PTR_ERR(ohandle_cache) : 0;
}

struct ohandle *ohandle_alloc(void)
{
	return mem_cache_alloc(ohandle_cache);
}

void ohandle_free(struct ohandle *oh)
{
	mem_cache_free(ohandle_cache, oh);
}

int ohandle_cmp(const void *va, const void *vb)
{
	const struct ohandle *a = va;
	const struct ohandle *b = vb;

	if (a->handle < b->handle)
		return -1;
	if (a->handle > b->handle)
		return 1;
	return 0;
}

uint32_t ohandle_insert(struct fsconn *conn, struct ohandle *oh)
{
	uint32_t handle;

	for (;;) {
		avl_index_t where;

		oh->handle = rand32();

		/* zero handles are not allowed */
		if (!oh->handle)
			continue;

		if (avl_find(&conn->open_handles, oh, &where))
			continue;

		/* handing off our reference */
		avl_insert(&conn->open_handles, oh, where);

		handle = oh->handle;
		break;
	}

	return handle;
}

void ohandle_remove(struct fsconn *conn, struct ohandle *oh)
{
	avl_remove(&conn->open_handles, oh);
}

struct ohandle *ohandle_find(struct fsconn *conn, const uint32_t handle)
{
	struct ohandle key = {
		.handle = handle,
	};

	return avl_find(&conn->open_handles, &key, NULL);
}

void ohandle_close_all(struct fsconn *conn)
{
	struct ohandle *oh;
	void *cookie;

	cookie = NULL;
	while ((oh = avl_destroy_nodes(&conn->open_handles, &cookie))) {
		int ret;

		ASSERT3P(conn->vol, !=, NULL);

		ret = objstore_close(conn->vol, oh->cookie);
		if (ret)
			cmn_err(CE_WARN, "conn %p failed to close_all cookie "
				"%p on vol %p: %s", conn, oh->cookie,
				conn->vol, xstrerror(ret));

		ohandle_free(oh);
	}
}
