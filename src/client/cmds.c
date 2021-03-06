/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * Copyright (c) 2015 Holly Sipek
 * Copyright (c) 2016 Steve Dougherty
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

#include <jeffpc/error.h>

#include <nomad/rpc_fs.h>

#include "cmds.h"

#define CMD(op, what, hndlr, login)			\
	{						\
		.name = #op,				\
		.opcode = (op),				\
		.handler = (hndlr),			\
		.requires_login = (login),		\
	}

#define CMD_ARG(op, what, hndlr, login)			\
	{						\
		.name = #op,				\
		.opcode = (op),				\
		.handler = (hndlr),			\
		.requires_login = (login),		\
		.reqoff = offsetof(union cmd, what.req),\
		.req = (void *) xdr_rpc_##what##_req,	\
	}

#define CMD_ARG_RET(op, what, hndlr, login)		\
	{						\
		.name = #op,				\
		.opcode = (op),				\
		.handler = (hndlr),			\
		.requires_login = (login),		\
		.reqoff = offsetof(union cmd, what.req),\
		.resoff = offsetof(union cmd, what.res),\
		.req = (void *) xdr_rpc_##what##_req,	\
		.res = (void *) xdr_rpc_##what##_res,	\
	}

static const struct cmdtbl {
	const char *name;
	uint16_t opcode;
	int (*handler)(struct fsconn *, union cmd *);
	bool requires_login;
	size_t reqoff;
	size_t resoff;

	/*
	 * We cheat a bit and make the arg void * even though it has a more
	 * specific type (e.g., xdr_login_req).
	 */
	bool_t (*req)(XDR *, void *);
	bool_t (*res)(XDR *, void *);
} cmdtbl[] = {
	CMD_ARG    (NRPC_CLOSE,         close,         cmd_close,       true),
	CMD_ARG_RET(NRPC_CREATE,        create,        cmd_create,      true),
	CMD_ARG_RET(NRPC_GETATTR,       getattr,       cmd_getattr,     true),
	CMD_ARG_RET(NRPC_GETDENT,       getdent,       cmd_getdent,     true),
	CMD_ARG_RET(NRPC_LOGIN,         login,         cmd_login,       false),
	CMD_ARG_RET(NRPC_LOOKUP,        lookup,        cmd_lookup,      true),
	CMD        (NRPC_NOP,           nop,           cmd_nop,         false),
	CMD_ARG_RET(NRPC_OPEN,          open,          cmd_open,        true),
	CMD_ARG_RET(NRPC_READ,          read,          cmd_read,        true),
	CMD_ARG_RET(NRPC_SETATTR,       setattr,       cmd_setattr,     true),
	CMD_ARG    (NRPC_UNLINK,        unlink,        cmd_unlink,      true),
	CMD_ARG_RET(NRPC_VDEV_IMPORT,	vdev_import,   cmd_vdev_import,	false),
	CMD_ARG    (NRPC_WRITE,         write,         cmd_write,       true),
};

static bool send_response(XDR *xdr, int fd, int err)
{
	struct rpc_header_res cmd;

	xdr_destroy(xdr);
	xdrfd_create(xdr, fd, XDR_ENCODE);

	cmd.err = errno_to_nerr(err);

	if (!xdr_rpc_header_res(xdr, &cmd))
		return false; /* failed to send */

	if (cmd.err != NERR_SUCCESS)
		return true; /* RPC failed */

	return true; /* all good */
}

static bool process_args(XDR *xdr, const struct cmdtbl *def, void *cmd)
{
	if (!def->req)
		return true; /* no args -> return success */

	if (!def->req(xdr, cmd + def->reqoff))
		return false;

	return true;
}

static bool process_returns(XDR *xdr, const struct cmdtbl *def, void *cmd)
{
	if (!def->res)
		return true; /* no response -> return success */

	if (!def->res(xdr, cmd + def->resoff))
		return false;

	return true;
}

bool process_handshake(struct fsconn *conn)
{
	struct rpc_handshake_req request;
	bool ok = false;
	XDR xdr;
	int ret;

	memset(&request, 0, sizeof(struct rpc_handshake_req));

	xdrfd_create(&xdr, conn->fd, XDR_DECODE);

	if (!xdr_rpc_handshake_req(&xdr, &request))
		goto out;

	if (request.vers == NRPC_VERSION) {
		ok = true;
		ret = 0;
	} else {
		ret = -ENOTSUP;
	}

	send_response(&xdr, conn->fd, ret);

out:
	xdr_destroy(&xdr);

	return ok;
}

bool process_connection(struct fsconn *conn)
{
	struct rpc_header_req hdr;
	union cmd cmd;
	bool ok = false;
	size_t i;
	int ret;
	XDR xdr;

	memset(&hdr, 0, sizeof(struct rpc_header_req));
	memset(&cmd, 0, sizeof(union cmd));

	xdrfd_create(&xdr, conn->fd, XDR_DECODE);

	if (!xdr_rpc_header_req(&xdr, &hdr))
		goto out;

	ret = -ENOTSUP;

	for (i = 0; i < ARRAY_LEN(cmdtbl); i++) {
		const struct cmdtbl *def = &cmdtbl[i];
		int ret;

		if (def->opcode != hdr.opcode)
			continue;

		/*
		 * we found the command handler
		 */
		cmn_err(CE_DEBUG, "opcode decoded as: %s", def->name);

		/* fetch arguments */
		ok = process_args(&xdr, def, &cmd);
		if (!ok) {
			cmn_err(CE_ERROR, "failed to fetch args");
			goto out;
		}

		/* if login is required, make sure it happened */
		if (def->requires_login && !conn->vol) {
			ret = -EPROTO;
			cmn_err(CE_ERROR, "must do LOGIN before this operation");
			break;
		}

		/* invoke the handler */
		ret = def->handler(conn, &cmd);

		/* free the arguments */
		xdr_destroy(&xdr);
		xdrfd_create(&xdr, conn->fd, XDR_FREE);
		process_args(&xdr, def, &cmd);
		xdr_destroy(&xdr);

		/* send back the response header */
		xdrfd_create(&xdr, conn->fd, XDR_ENCODE);
		ok = send_response(&xdr, conn->fd, ret);

		/* send back the response payload */
		if (ok && !ret) {
			ok = process_returns(&xdr, def, &cmd);

			/* free the responses */
			xdr_destroy(&xdr);
			xdrfd_create(&xdr, conn->fd, XDR_FREE);
			process_returns(&xdr, def, &cmd);
		}

		goto out;
	}

	if (i == ARRAY_LEN(cmdtbl))
		cmn_err(CE_DEBUG, "unknown opcode: %u", hdr.opcode);

	send_response(&xdr, conn->fd, ret);

out:
	xdr_destroy(&xdr);

	return ok;
}
