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

#ifndef __NOMAD_RPC_H
#define __NOMAD_RPC_H

#include <rpc/rpc.h>

/*
 * RPC error codes
 *
 * Note: When adding new error codes, don't forget to update errno_to_nerr
 * in error.c!
 */
#define NERR_UNKNOWN_ERROR     -1
#define NERR_SUCCESS           0
#define NERR_EPERM             1
#define NERR_ENOENT            2
#define NERR_ESRCH             3
#define NERR_EINTR             4
#define NERR_EIO               5
#define NERR_ENXIO             6
#define NERR_E2BIG             7
#define NERR_EBADF             9
#define NERR_EAGAIN            11
#define NERR_ENOMEM            12
#define NERR_EACCES            13
#define NERR_EFAULT            14
#define NERR_EBUSY             16
#define NERR_EEXIST            17
#define NERR_EXDEV             18
#define NERR_ENODEV            19
#define NERR_ENOTDIR           20
#define NERR_EISDIR            21
#define NERR_EINVAL            22
#define NERR_ENOSPC            28
#define NERR_ENOTSUP           48
#define NERR_EPROTO            71
#define NERR_EOVERFLOW         79
#define NERR_ENOTUNIQ          80
#define NERR_EALREADY          149

extern int errno_to_nerr(int e);

extern void xdrfd_create(XDR *xdr, int fd, enum xdr_op op);

#endif
