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

#include "fscall.h"

#include <jeffpc/sock.h>

#include <nomad/rpc_fs.h>

static int __fscall_req(int fd, uint32_t opcode,
			int (*xmit)(XDR *, void *),
			void *req)
{
	struct rpc_header_req header;
	XDR xdr;
	int ret;

	header.opcode = opcode;

	xdrfd_create(&xdr, fd, XDR_ENCODE);

	ret = NERR_RPC_ERROR;

	/* send generic RPC header */
	if (!xdr_rpc_header_req(&xdr, &header))
		goto err;

	if (xmit && !xmit(&xdr, req))
		goto err;

	ret = 0;

err:
	xdr_destroy(&xdr);

	return ret;
}

static int __fscall_res(int fd,
			int (*xmit)(XDR *, void *),
			void *res, size_t ressize)
{
	struct rpc_header_res header;
	XDR xdr;
	int ret;

	if (res)
		memset(res, 0, ressize);

	xdrfd_create(&xdr, fd, XDR_DECODE);

	ret = NERR_RPC_ERROR;

	if (!xdr_rpc_header_res(&xdr, &header))
		goto err;

	ret = header.err;
	if (ret)
		goto err;

	if (xmit && !xmit(&xdr, res))
		ret = NERR_RPC_ERROR;
	else
		ret = 0;

err:
	xdr_destroy(&xdr);

	return ret;
}

static int __fscall(int fd, uint32_t opcode,
		    int (*reqxmit)(XDR *, void *),
		    int (*resxmit)(XDR *, void *),
		    void *req, void *res,
		    size_t ressize)
{
	int ret;

	ret = __fscall_req(fd, opcode, reqxmit, req);
	if (ret)
		return ret;

	return __fscall_res(fd, resxmit, res, ressize);
}

int fscall_login(struct fscall_state *state, const char *conn, const char *vg)
{
	struct rpc_login_req login_req;
	struct rpc_login_res login_res;
	int ret;

	login_req.conn = (char *) conn;
	login_req.vg = (char *) vg;

	ret = __fscall(state->sock, NRPC_LOGIN,
		       (void *) xdr_rpc_login_req,
		       (void *) xdr_rpc_login_res,
		       &login_req,
		       &login_res,
		       sizeof(login_res));
	if (ret)
		return ret;

	state->root_oid = login_res.root;

	return 0;
}

int fscall_open(struct fscall_state *state, const struct noid *oid,
		uint32_t *handle)
{
	struct rpc_open_req open_req;
	struct rpc_open_res open_res;
	int ret;

	open_req.oid = *oid;
	memset(&open_req.clock, 0, sizeof(open_req.clock));

	ret = __fscall(state->sock, NRPC_OPEN,
		       (void *) xdr_rpc_open_req,
		       (void *) xdr_rpc_open_res,
		       &open_req,
		       &open_res,
		       sizeof(open_res));
	if (ret)
		return ret;

	*handle = open_res.handle;

	return 0;
}

int fscall_close(struct fscall_state *state, const uint32_t handle)
{
	struct rpc_close_req close_req;

	close_req.handle = handle;

	return __fscall(state->sock, NRPC_CLOSE,
			(void *) xdr_rpc_close_req,
			NULL,
			&close_req,
			NULL,
			0);
}

int fscall_getattr(struct fscall_state *state, const uint32_t handle,
		   struct nattr *attr)
{
	struct rpc_getattr_req getattr_req;
	struct rpc_getattr_res getattr_res;
	int ret;

	getattr_req.handle = handle;

	ret = __fscall(state->sock, NRPC_GETATTR,
		       (void *) xdr_rpc_getattr_req,
		       (void *) xdr_rpc_getattr_res,
		       &getattr_req,
		       &getattr_res,
		       sizeof(getattr_res));
	if (ret)
		return ret;

	*attr = getattr_res.attr;

	return 0;
}

int fscall_setattr(struct fscall_state *state, const uint32_t handle,
		   struct nattr *attr, bool size_is_valid,
		   bool mode_is_valid)
{
	struct rpc_setattr_req setattr_req;
	struct rpc_setattr_res setattr_res;
	int ret;

	setattr_req.handle = handle;
	setattr_req.attr = *attr;
	setattr_req.size_is_valid = size_is_valid;
	setattr_req.mode_is_valid = mode_is_valid;

	ret = __fscall(state->sock, NRPC_SETATTR,
		       (void *) xdr_rpc_setattr_req,
		       (void *) xdr_rpc_setattr_res,
		       &setattr_req,
		       &setattr_res,
		       sizeof(setattr_res));
	if (ret)
		return ret;

	*attr = setattr_res.attr;

	return 0;
}

int fscall_lookup(struct fscall_state *state, const uint32_t parent_handle,
		  const char *name, struct noid *child)
{
	struct rpc_lookup_req lookup_req;
	struct rpc_lookup_res lookup_res;
	int ret;

	lookup_req.parent = parent_handle;
	lookup_req.path = (char *) name;

	ret = __fscall(state->sock, NRPC_LOOKUP,
		       (void *) xdr_rpc_lookup_req,
		       (void *) xdr_rpc_lookup_res,
		       &lookup_req,
		       &lookup_res,
		       sizeof(lookup_res));
	if (ret)
		return ret;

	*child = lookup_res.child;

	return 0;
}

int fscall_create(struct fscall_state *state, const uint32_t parent_handle,
		  const char *name, const uint16_t mode, struct noid *child)
{
	struct rpc_create_req create_req;
	struct rpc_create_res create_res;
	int ret;

	create_req.parent = parent_handle;
	create_req.path = (char *) name;
	create_req.mode = mode;

	ret = __fscall(state->sock, NRPC_CREATE,
		       (void *) xdr_rpc_create_req,
		       (void *) xdr_rpc_create_res,
		       &create_req,
		       &create_res,
		       sizeof(create_res));
	if (ret)
		return ret;

	*child = create_res.oid;

	return 0;
}

int fscall_read(struct fscall_state *state, const uint32_t handle,
		void *buf, size_t len, uint64_t off)
{
	struct rpc_read_req read_req;
	struct rpc_read_res read_res;
	int ret;

	/* TODO: check for length being too much? */

	read_req.handle = handle;
	read_req.offset = off;
	read_req.length = len;

	ret = __fscall(state->sock, NRPC_READ,
		       (void *) xdr_rpc_read_req,
		       (void *) xdr_rpc_read_res,
		       &read_req,
		       &read_res,
		       sizeof(read_res));
	if (ret)
		return ret;

	memcpy(buf, read_res.data.data_val, read_res.data.data_len);

	return 0;
}

int fscall_write(struct fscall_state *state, const uint32_t handle,
		 const void *buf, size_t len, uint64_t off)
{
	struct rpc_write_req write_req;

	/* TODO: check for length being too much? */

	write_req.handle = handle;
	write_req.offset = off;
	write_req.data.data_len = len;
	write_req.data.data_val = (void *) buf;

	return __fscall(state->sock, NRPC_WRITE,
			(void *) xdr_rpc_write_req,
			NULL,
			&write_req,
			NULL,
			0);
}

int fscall_getdent(struct fscall_state *state, const uint32_t handle,
		   const uint64_t off, struct noid *oid, char **name,
		   uint64_t *entry_size)
{
	struct rpc_getdent_req getdent_req;
	struct rpc_getdent_res getdent_res;
	int ret;

	getdent_req.parent = handle;
	getdent_req.offset = off;

	ret = __fscall(state->sock, NRPC_GETDENT,
		       (void *) xdr_rpc_getdent_req,
		       (void *) xdr_rpc_getdent_res,
		       &getdent_req,
		       &getdent_res,
		       sizeof(getdent_res));
	if (ret)
		return ret;

	*oid = getdent_res.oid;
	*name = getdent_res.name;
	*entry_size = getdent_res.entry_size;

	return 0;
}

int fscall_connect(const char *host, uint16_t port, const char *vg,
		   struct fscall_state *state)
{
	int ret;

	ret = connect_ip(host, port, true, true, IP_TCP);
	cmn_err(CE_DEBUG, "connect_ip() = %d", ret);
	if (ret < 0)
		return ret;

	state->sock = ret;

	ret = fscall_login(state, "unused", vg);
	cmn_err(CE_DEBUG, "fscall_login() = %d", ret);
	if (ret)
		return ret;

	ret = fscall_open(state, &state->root_oid, &state->root_ohandle);
	cmn_err(CE_DEBUG, "fscall_open() = %d", ret);
	if (ret)
		return ret;

	return 0;
}
