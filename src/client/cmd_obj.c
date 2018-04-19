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

int cmd_open(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_open_req *req = &cmd->open.req;
	struct rpc_open_res *res = &cmd->open.res;
	struct ohandle *oh;
	int ret;

	oh = ohandle_alloc();
	if (!oh)
		return -ENOMEM;

	oh->cookie = objstore_open(conn->vol, &req->oid, &req->clock);
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

	ret = objstore_close(conn->vol, oh->cookie);

	if (!ret) {
		ohandle_remove(conn, oh);
		ohandle_free(oh);
	}

	return ret;
}

int cmd_read(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_read_req *req = &cmd->read.req;
	struct rpc_read_res *res = &cmd->read.res;
	struct ohandle *oh;
	ssize_t ret;
	void *buf;

	oh = ohandle_find(conn, req->handle);
	if (!oh)
		return -EINVAL;

	/* TODO: should we limit the requested read size? */

	buf = malloc(req->length);
	if (!buf)
		return -ENOMEM;

	ret = objstore_read(conn->vol, oh->cookie, buf, req->length,
			    req->offset);
	if (ret < 0) {
		free(buf);
		return ret;
	}

	res->data.data_len = ret;
	res->data.data_val = buf;

	return 0;
}

int cmd_write(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_write_req *req = &cmd->write.req;
	struct ohandle *oh;
	ssize_t ret;

	oh = ohandle_find(conn, req->handle);
	if (!oh)
		return -EINVAL;

	/* TODO: should we limit the requested write size? */

	ret = objstore_write(conn->vol, oh->cookie, req->data.data_val,
			     req->data.data_len, req->offset);

	VERIFY3S(ret, ==, req->data.data_len);

	return (ret < 0) ? ret : 0;
}

int cmd_getattr(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_getattr_req *req = &cmd->getattr.req;
	struct rpc_getattr_res *res = &cmd->getattr.res;
	struct ohandle *oh;

	oh = ohandle_find(conn, req->handle);
	if (!oh)
		return -EINVAL;

	return objstore_getattr(conn->vol, oh->cookie, &res->attr);
}

int cmd_setattr(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_setattr_req *req = &cmd->setattr.req;
	struct rpc_setattr_res *res = &cmd->setattr.res;
	struct ohandle *oh;
	unsigned valid;

	oh = ohandle_find(conn, req->handle);
	if (!oh)
		return -EINVAL;

	valid = 0;
	valid |= req->mode_is_valid ? OBJ_ATTR_MODE : 0;
	valid |= req->size_is_valid ? OBJ_ATTR_SIZE : 0;

	/* we use the same struct for input and output */
	res->attr = req->attr;

	return objstore_setattr(conn->vol, oh->cookie, &res->attr, valid);
}
