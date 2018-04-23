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

#ifndef __NOMAD_FSCALL_H
#define __NOMAD_FSCALL_H

#include <jeffpc/int.h>
#include <jeffpc/uuid.h>

#include <nomad/types.h>

struct fscall_state {
	int sock;

	/* the root */
	struct noid root_oid;
	uint32_t root_ohandle;
};

extern int fscall_login(struct fscall_state *state, const char *conn,
			const struct xuuid *volid);
extern int fscall_open(struct fscall_state *state, const struct noid *oid,
		       uint32_t *handle);
extern int fscall_close(struct fscall_state *state, const uint32_t handle);
extern int fscall_getattr(struct fscall_state *state, const uint32_t handle,
			  struct nattr *attr);
extern int fscall_setattr(struct fscall_state *state, const uint32_t handle,
			  struct nattr *attr, bool size_is_valid,
			  bool mode_is_valid);
extern int fscall_lookup(struct fscall_state *state,
			 const uint32_t parent_handle, const char *name,
			 struct noid *child);
extern int fscall_create(struct fscall_state *state, const uint32_t parent_handle,
			 const char *name, const uint16_t mode, struct noid *child);
extern int fscall_read(struct fscall_state *state, const uint32_t handle,
		       void *buf, size_t len, uint64_t off);
extern int fscall_write(struct fscall_state *state, const uint32_t handle,
			const void *buf, size_t len, uint64_t off);
extern int fscall_getdent(struct fscall_state *state, const uint32_t handle,
			  const uint64_t off, struct noid *oid, char **name,
			  uint64_t *entry_size);
extern int fscall_vdev_import(struct fscall_state *state, const char *type,
			      const char *path, bool create,
			      struct xuuid *uuid);

extern int fscall_connect(struct fscall_state *state, int fd);
extern void fscall_disconnect(struct fscall_state *state);
extern int fscall_mount(struct fscall_state *state, const struct xuuid *volid);

#endif
