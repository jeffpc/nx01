/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

int cmd_create(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_create_req *req = &cmd->create.req;
	struct rpc_create_res *res = &cmd->create.res;

	return objstore_create(conn->vg, &req->parent_oid,
			       &req->parent_clock, req->path, req->mode,
			       &res->oid);
}

int cmd_lookup(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_lookup_req *req = &cmd->lookup.req;
	struct rpc_lookup_res *res = &cmd->lookup.res;

	return objstore_lookup(conn->vg, &req->parent_oid,
			       &req->parent_clock, req->path, &res->child);
}

int cmd_remove(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_remove_req *req = &cmd->remove.req;

	return objstore_remove(conn->vg, &req->parent_oid,
			       &req->parent_clock, req->path);
}
