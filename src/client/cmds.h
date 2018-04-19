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

#ifndef __NOMAD_CLIENT_CMDS_H
#define __NOMAD_CLIENT_CMDS_H

#include <sys/avl.h>

#include <nomad/rpc_fs.h>
#include <nomad/objstore.h>

union cmd {
	/* close */
	struct {
		struct rpc_close_req req;
	} close;

	/* create */
	struct {
		struct rpc_create_req req;
		struct rpc_create_res res;
	} create;

	/* getattr */
	struct {
		struct rpc_getattr_req req;
		struct rpc_getattr_res res;
	} getattr;

	/* getdent */
	struct {
		struct rpc_getdent_req req;
		struct rpc_getdent_res res;
	} getdent;

	/* login */
	struct {
		struct rpc_login_req req;
		struct rpc_login_res res;
	} login;

	/* lookup */
	struct {
		struct rpc_lookup_req req;
		struct rpc_lookup_res res;
	} lookup;

	/* nop - no req & no res */

	/* open */
	struct {
		struct rpc_open_req req;
		struct rpc_open_res res;
	} open;

	/* read */
	struct {
		struct rpc_read_req req;
		struct rpc_read_res res;
	} read;

	/* setattr */
	struct {
		struct rpc_setattr_req req;
		struct rpc_setattr_res res;
	} setattr;

	/* unlink */
	struct {
		struct rpc_unlink_req req;
	} unlink;

	/* write */
	struct {
		struct rpc_write_req req;
	} write;
};

struct fsconn {
	int fd;
	struct objstore *vol;

	avl_tree_t open_handles;
};

extern bool process_handshake(struct fsconn *conn);
extern bool process_connection(struct fsconn *conn);

/* RPC handlers */
extern int cmd_close(struct fsconn *conn, union cmd *cmd);
extern int cmd_create(struct fsconn *conn, union cmd *cmd);
extern int cmd_getattr(struct fsconn *conn, union cmd *cmd);
extern int cmd_getdent(struct fsconn *conn, union cmd *cmd);
extern int cmd_login(struct fsconn *conn, union cmd *cmd);
extern int cmd_lookup(struct fsconn *conn, union cmd *cmd);
extern int cmd_nop(struct fsconn *conn, union cmd *cmd);
extern int cmd_open(struct fsconn *conn, union cmd *cmd);
extern int cmd_read(struct fsconn *conn, union cmd *cmd);
extern int cmd_setattr(struct fsconn *conn, union cmd *cmd);
extern int cmd_unlink(struct fsconn *conn, union cmd *cmd);
extern int cmd_write(struct fsconn *conn, union cmd *cmd);

#endif
