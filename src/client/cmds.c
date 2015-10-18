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

#include <stddef.h>

#include <nomad/error.h>
#include <nomad/rpc_fs.h>

#include "cmds.h"

#define CMD(op, what, hndlr)				\
	{						\
		.name = #op,				\
		.opcode = (op),				\
		.handler = (hndlr),			\
	}

#define CMD_ARG_RET(op, what, hndlr)			\
	{						\
		.name = #op,				\
		.opcode = (op),				\
		.handler = (hndlr),			\
		.reqoff = offsetof(union cmd, what.req),\
		.resoff = offsetof(union cmd, what.res),\
		.req = (void *) xdr_rpc_##what##_req,	\
		.res = (void *) xdr_rpc_##what##_res,	\
	}

static const struct cmdtbl {
	const char *name;
	uint16_t opcode;
	int (*handler)(union cmd *);
	size_t reqoff;
	size_t resoff;

	/*
	 * We cheat a bit and make the arg void * even though it has a more
	 * specific type (e.g., xdr_login_req).
	 */
	bool_t (*req)(XDR *, void *);
	bool_t (*res)(XDR *, void *);
} cmdtbl[] = {
	CMD_ARG_RET(NRPC_LOGIN,         login,         cmd_login),
	CMD        (NRPC_NOP,           nop,           cmd_nop),
	CMD_ARG_RET(NRPC_STAT,          stat,          cmd_stat),
};

#define MAP_ERRNO(errno)		\
	case errno:			\
		cmd.err = NERR_##errno;	\
		break

static bool send_response(XDR *xdr, int fd, int err)
{
	struct rpc_header_res cmd;
	int ret;

	switch (err) {
		MAP_ERRNO(ENOENT);
		MAP_ERRNO(EEXIST);
		case 0:
			cmd.err = NERR_SUCCESS;
			break;
		default:
			fprintf(stderr, "%s cannot map errno %d (%s) to NERR_*\n",
				__func__, err, strerror(err));
			cmd.err = NERR_UNKNOWN_ERROR;
			break;
	}

	xdr_destroy(xdr);
	xdrfd_create(xdr, fd, XDR_ENCODE);

	ret = xdr_rpc_header_res(xdr, &cmd);
	if (!ret)
		return false; /* failed to send */

	if (cmd.err != NERR_SUCCESS)
		return false; /* RPC failed */

	return true; /* all good */
}

static bool fetch_args(XDR *xdr, const struct cmdtbl *def, void *cmd)
{
	if (!def->req(xdr, cmd + def->reqoff))
		return false;

	return true;
}

static bool send_returns(XDR *xdr, const struct cmdtbl *def, void *cmd)
{
	if (!def->res(xdr, cmd + def->resoff))
		return false;

	return true;
}

bool process_connection(int fd)
{
	struct rpc_header_req hdr;
	union cmd cmd;
	bool ok = false;
	size_t i;
	XDR xdr;

	memset(&hdr, 0, sizeof(struct rpc_header_req));
	memset(&cmd, 0, sizeof(union cmd));

	xdrfd_create(&xdr, fd, XDR_DECODE);

	if (!xdr_rpc_header_req(&xdr, &hdr))
		goto out;

	printf("got opcode %u\n", hdr.opcode);

	for (i = 0; i < ARRAY_LEN(cmdtbl); i++) {
		const struct cmdtbl *def = &cmdtbl[i];
		int ret;

		if (def->opcode != hdr.opcode)
			continue;

		/*
		 * we found the command handler
		 */
		printf("opcode decoded as: %s\n", def->name);

		/* fetch arguments */
		if (def->req) {
			ok = fetch_args(&xdr, def, &cmd);
			if (!ok) {
				printf("failed to fetch args\n");
				goto out;
			}
		}

		/* invoke the handler */
		ret = def->handler(&cmd);

		/* send back the response header */
		ok = send_response(&xdr, fd, ret);

		/* send back the response payload */
		if (ok && def->res)
			ok = send_returns(&xdr, def, &cmd);

		goto out;
	}

	send_response(&xdr, fd, ENOTSUP);

out:
	xdr_destroy(&xdr);

	return ok;
}
