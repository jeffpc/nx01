/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * Copyright (c) 2015 Holly Sipek
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

#include <jeffpc/error.h>

#include "cmds.h"
#include "ohandle.h"

int cmd_create(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_create_req *req = &cmd->create.req;
	struct rpc_create_res *res = &cmd->create.res;
	struct ohandle *oh;

	oh = ohandle_find(conn, req->parent);
	if (!oh)
		return -EINVAL;

	return objstore_create(conn->vol, oh->cookie, req->path, req->mode,
			       &res->oid);
}

int cmd_lookup(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_lookup_req *req = &cmd->lookup.req;
	struct rpc_lookup_res *res = &cmd->lookup.res;
	struct ohandle *oh;

	oh = ohandle_find(conn, req->parent);
	if (!oh)
		return -EINVAL;

	return objstore_lookup(conn->vol, oh->cookie, req->path, &res->child);
}

int cmd_unlink(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_unlink_req *req = &cmd->unlink.req;
	struct ohandle *oh;

	oh = ohandle_find(conn, req->parent);
	if (!oh)
		return -EINVAL;

	return objstore_unlink(conn->vol, oh->cookie, req->path);
}

int cmd_getdent(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_getdent_req *req = &cmd->getdent.req;
	struct rpc_getdent_res *res = &cmd->getdent.res;
	struct ohandle *oh;

	oh = ohandle_find(conn, req->parent);
	if (!oh)
		return -EINVAL;

	return objstore_getdent(conn->vol, oh->cookie, req->offset,
				&res->oid, &res->name, &res->entry_size);
}
