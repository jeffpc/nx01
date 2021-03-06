/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
 * and nerr_to_errno in error.c!
 *
 * NERR_RPC_ERROR and NERR_UNKNOWN_ERROR are special.  While all other codes
 * map 1:1 to standard errno's, these two are used in very specific
 * situations:
 *
 *   NERR_UNKNOWN_ERROR:
 *       Used when the server has trouble mapping an errno to an NERR_*
 *       value.  Ideally, we should never see this error on the client as
 *       there should be an NERR_* value defined for every errno.
 *
 *   NERR_RPC_ERROR:
 *       An error occured inside the RPC layers.  They cannot return an NERR
 *       since the caller then wouldn't know if the error originated on the
 *       client or on the server.
 */
#define NERR_RPC_ERROR         0xfffe
#define NERR_UNKNOWN_ERROR     0xffff

#define NERR_SUCCESS           0
#define NERR_EPERM             1
#define NERR_ENOENT            2
#define NERR_ESRCH             3
#define NERR_EINTR             4
#define NERR_EIO               5
#define NERR_ENXIO             6
#define NERR_E2BIG             7
#define	NERR_ENOEXEC           8
#define NERR_EBADF             9
#define	NERR_ECHILD            10
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
#define	NERR_ENFILE            23
#define	NERR_EMFILE            24
#define	NERR_ENOTTY            25
#define	NERR_EFBIG             27
#define NERR_ENOSPC            28
#define	NERR_ESPIPE            29
#define	NERR_EROFS             30
#define	NERR_EMLINK            31
#define	NERR_EPIPE             32
#define	NERR_EDOM              33
#define	NERR_ERANGE            34
#define	NERR_EDEADLK           45
#define	NERR_ENOLCK            46
#define	NERR_ECANCELED         47
#define NERR_ENOTSUP           48
#define NERR_EPROTO            71
#define NERR_EOVERFLOW         79
#define NERR_ENOTUNIQ          80
#define NERR_EALREADY          149

extern int errno_to_nerr(int e);
extern int nerr_to_errno(int e);

extern void xdrfd_create(XDR *xdr, int fd, enum xdr_op op);

#endif
