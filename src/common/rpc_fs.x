/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * Copyright (c) 2015 Holly Sipek
 * Copyright (c) 2015 Joshua Kahn <josh@joshuak.net>
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

%#include <nomad/types.h>

#define HANDLE(n)	uint32_t	n

%/***** RPC header *****/
struct rpc_header_req {
	uint16_t	opcode;
};

struct rpc_header_res {
	uint32_t	err;
};

%/***** LOGIN *****/
struct rpc_login_req {
	string		conn<>;
	string		vg<>;
};

struct rpc_login_res {
	struct noid	root;
};

%/***** STAT *****/
struct rpc_stat_req {
	struct noid	oid;
	struct nvclock	clock;
};

struct rpc_stat_res {
	struct nattr	attr;
};

%/***** LOOKUP *****/
struct rpc_lookup_req {
	struct noid	parent_oid;
	struct nvclock	parent_clock;
	string		path<>;
};

struct rpc_lookup_res {
	struct noid	child;
};

%/***** CREATE *****/
struct rpc_create_req {
	HANDLE(parent);
	string		path<>;
	uint16_t	mode; /* see NATTR_* in common/include/nomad/atrr.h */
};

struct rpc_create_res {
	struct noid	oid;
};

%/***** REMOVE *****/
struct rpc_remove_req {
	HANDLE(parent);
	string		path<>;
};

%/***** OPEN *****/
struct rpc_open_req {
	struct noid	oid;
	struct nvclock	clock;
};

struct rpc_open_res {
	HANDLE(handle);
};

%/***** CLOSE *****/
struct rpc_close_req {
	HANDLE(handle);
};
