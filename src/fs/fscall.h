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

#include <jeffpc/int.h>

#include <nomad/types.h>

struct fscall_state {
	int sock;

	/* the root */
	struct noid root_oid;
	uint32_t root_ohandle;
};

extern int fscall_login(struct fscall_state *state, const char *conn,
			const char *vg);
extern int fscall_open(struct fscall_state *state, const struct noid *oid,
		       uint32_t *handle);
extern int fscall_close(struct fscall_state *state, const uint32_t handle);
extern int fscall_getattr(struct fscall_state *state, const uint32_t handle,
			  struct nattr *attr);
extern int fscall_lookup(struct fscall_state *state,
			 const uint32_t parent_handle, const char *name,
			 struct noid *child);
extern int fscall_read(struct fscall_state *state, const uint32_t handle,
		       void *buf, size_t len, uint64_t off);
extern int fscall_getdent(struct fscall_state *state, const uint32_t handle,
			  const uint64_t off, struct noid *oid, char **name,
			  uint64_t *entry_size);

extern int fscall_connect(const char *host, uint16_t port, const char *vg,
			  struct fscall_state *state);
