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
#include "ohandle.h"

int cmd_open(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_open_req *req = &cmd->open.req;
	struct rpc_open_res *res = &cmd->open.res;
	struct ohandle *oh;
	int ret;

	oh = ohandle_alloc();
	if (!oh)
		return -ENOMEM;

	oh->cookie = objstore_open(conn->vg, &req->oid, &req->clock);
	if (IS_ERR(oh->cookie))
		goto err;

	res->handle = ohandle_insert(conn, oh);

	return 0;

err:
	ret = PTR_ERR(oh->cookie);

	ohandle_free(oh);

	return ret;
}

int cmd_close(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_close_req *req = &cmd->close.req;
	struct ohandle *oh;
	int ret;

	oh = ohandle_find(conn, req->handle);
	if (!oh)
		return -EINVAL;

	ret = objstore_close(conn->vg, oh->cookie);

	if (!ret) {
		ohandle_remove(conn, oh);
		ohandle_free(oh);
	}

	return ret;
}

int cmd_stat(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_stat_req *req = &cmd->stat.req;
	struct rpc_stat_res *res = &cmd->stat.res;
	struct ohandle *oh;

	oh = ohandle_find(conn, req->handle);
	if (!oh)
		return -EINVAL;

	return objstore_getattr(conn->vg, oh->cookie, &res->attr);
}
