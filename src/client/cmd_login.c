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

#include <nomad/types.h>
#include <nomad/objstore.h>

#include "cmds.h"

int cmd_login(struct fsconn *conn, union cmd *cmd)
{
	struct rpc_login_req *req = &cmd->login.req;
	struct rpc_login_res *res = &cmd->login.res;
	struct objstore *vol;

	cmn_err(CE_DEBUG, "LOGIN: conn = '%s', volname = '%s'", req->conn,
		req->volname);

	if (conn->vol) {
		cmn_err(CE_INFO, "LOGIN: error: this connection "
			"already logged in.");
		return -EALREADY;
	}

	vol = objstore_vol_lookup(req->volname);
	if (!vol)
		return -ENOENT;

	conn->vol = vol;

	return objstore_getroot(vol, &res->root);
}
