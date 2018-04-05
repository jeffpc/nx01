/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * Copyright (c) 2015 Holly Sipek
 * Copyright (c) 2015 Joshua Kahn <josh@joshuak.net>
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

%#include <nomad/types.h>

#define HANDLE(n)	uint32_t	n

%/***** HANDSHAKE ******/
struct rpc_handshake_req {
	uint32_t	vers;
};

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
	string		pool<>;
};

struct rpc_login_res {
	struct noid	root;
};

%/***** GETATTR *****/
struct rpc_getattr_req {
	HANDLE(handle);
};

struct rpc_getattr_res {
	struct nattr	attr;
};

%/***** LOOKUP *****/
struct rpc_lookup_req {
	HANDLE(parent);
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

%/***** UNLINK *****/
struct rpc_unlink_req {
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

%/***** READ *****/
struct rpc_read_req {
	HANDLE(handle);
	uint64_t offset;
	/*
	 * 32-bit length is ok because rpcgen doesn't know how to send more
	 * than 4GB anyway.  Furthermore, the object store has a 31-bit
	 * length limitation on 32-bit systems.
	 */
	uint32_t length;
};

struct rpc_read_res {
	/* length is sent as part of data */
	opaque data<4294967295>;
};

%/***** WRITE *****/
struct rpc_write_req {
	HANDLE(handle);
	uint64_t offset;
	/* length is sent as part of data */
	opaque data<4294967295>;
};

%/***** SETATTR *****/
struct rpc_setattr_req {
	HANDLE(handle);
	struct nattr attr;
	bool size_is_valid;
	bool mode_is_valid;
};

struct rpc_setattr_res {
	struct nattr attr;
};

%/***** GETDENT *****/
struct rpc_getdent_req {
	HANDLE(parent);
	uint64_t offset;
};

struct rpc_getdent_res {
	struct noid oid;
	string name<>;
	uint64_t entry_size;
};
