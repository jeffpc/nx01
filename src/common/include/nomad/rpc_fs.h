/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __NOMAD_RPC_FS_H
#define __NOMAD_RPC_FS_H

#include <nomad/rpc_fs_xdr.h>
#include <nomad/rpc.h>

#define NRPC_VERSION 0x00000001

#define NRPC_NOP		0x0000
#define NRPC_LOGIN		0x0001
#define NRPC_GETATTR		0x0002
#define NRPC_LOOKUP		0x0003
#define NRPC_CREATE		0x0004
#define NRPC_UNLINK		0x0005
#define NRPC_OPEN		0x0006
#define NRPC_CLOSE		0x0007
#define NRPC_READ		0x0008
#define NRPC_WRITE		0x0009
#define NRPC_SETATTR		0x000A
#define NRPC_GETDENT		0x000B

#endif
