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

#ifndef __NOMAD_CLIENT_CMDS_H
#define __NOMAD_CLIENT_CMDS_H

#include <nomad/rpc_fs.h>

union cmd {
	/* create */
	struct {
		struct rpc_create_req req;
		struct rpc_create_res res;
	} create;

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

	/* remove */
	struct {
		struct rpc_remove_req req;
	} remove;

	/* stat */
	struct {
		struct rpc_stat_req req;
		struct rpc_stat_res res;
	} stat;
};

extern bool process_connection(int fd);

/* RPC handlers */
extern int cmd_create(union cmd *cmd);
extern int cmd_login(union cmd *cmd);
extern int cmd_lookup(union cmd *cmd);
extern int cmd_nop(union cmd *cmd);
extern int cmd_remove(union cmd *cmd);
extern int cmd_stat(union cmd *cmd);

#endif
