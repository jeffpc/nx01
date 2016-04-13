/*
 * Copyright (c) 2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __NOMAD_CLIENT_OHANDLE_H
#define __NOMAD_CLIENT_OHANDLE_H

#include <sys/avl.h>

#include "cmds.h"

struct ohandle {
	/* key */
	uint32_t handle;

	/* value */
	void *cookie;

	/* misc */
	avl_node_t node;
};

extern int ohandle_cmp(const void *va, const void *vb);
extern uint32_t ohandle_insert(struct fsconn *conn, struct ohandle *oh);
extern void ohandle_remove(struct fsconn *conn, struct ohandle *oh);
extern struct ohandle *ohandle_find(struct fsconn *conn, const uint32_t handle);
extern void ohandle_close_all(struct fsconn *conn);

static inline struct ohandle *ohandle_alloc(void)
{
	return malloc(sizeof(struct ohandle));
}

static inline void ohandle_free(struct ohandle *oh)
{
	free(oh);
}

#endif
