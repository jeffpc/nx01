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

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <pthread.h>

#include <jeffpc/error.h>

#include <nomad/rpc.h>

#define MAP_ERRNO(errno)			\
	case -errno: return NERR_##errno

int errno_to_nerr(int e)
{
	VERIFY3S(e, <=, 0);

	switch (e) {
		MAP_ERRNO(E2BIG);
		MAP_ERRNO(EACCES);
		MAP_ERRNO(EAGAIN);
		MAP_ERRNO(EALREADY);
		MAP_ERRNO(EBADF);
		MAP_ERRNO(EBUSY);
		MAP_ERRNO(EEXIST);
		MAP_ERRNO(EFAULT);
		MAP_ERRNO(EINTR);
		MAP_ERRNO(EINVAL);
		MAP_ERRNO(EIO);
		MAP_ERRNO(EISDIR);
		MAP_ERRNO(ENODEV);
		MAP_ERRNO(ENOENT);
		MAP_ERRNO(ENOMEM);
		MAP_ERRNO(ENOSPC);
		MAP_ERRNO(ENOTDIR);
		MAP_ERRNO(ENOTSUP);
		MAP_ERRNO(ENOTUNIQ);
		MAP_ERRNO(ENXIO);
		MAP_ERRNO(EOVERFLOW);
		MAP_ERRNO(EPERM);
		MAP_ERRNO(EPROTO);
		MAP_ERRNO(ESRCH);
		MAP_ERRNO(EXDEV);
		case 0:
			return NERR_SUCCESS;
		/* there is no errno that maps to NERR_RPC_ERROR */
	}

	return NERR_UNKNOWN_ERROR;
}

#define MAP_NERR(nerr)				\
	case NERR_##nerr: return -nerr

int nerr_to_errno(int e)
{
	switch (e) {
		MAP_NERR(E2BIG);
		MAP_NERR(EACCES);
		MAP_NERR(EAGAIN);
		MAP_NERR(EALREADY);
		MAP_NERR(EBADF);
		MAP_NERR(EBUSY);
		MAP_NERR(EEXIST);
		MAP_NERR(EFAULT);
		MAP_NERR(EINTR);
		MAP_NERR(EINVAL);
		MAP_NERR(EIO);
		MAP_NERR(EISDIR);
		MAP_NERR(ENODEV);
		MAP_NERR(ENOENT);
		MAP_NERR(ENOMEM);
		MAP_NERR(ENOSPC);
		MAP_NERR(ENOTDIR);
		MAP_NERR(ENOTSUP);
		MAP_NERR(ENOTUNIQ);
		MAP_NERR(ENXIO);
		MAP_NERR(EOVERFLOW);
		MAP_NERR(EPERM);
		MAP_NERR(EPROTO);
		MAP_NERR(ESRCH);
		MAP_NERR(EXDEV);

		/* special cases */
		case NERR_SUCCESS:
			return 0;
		case NERR_UNKNOWN_ERROR:
			return -EPROTO;
		case NERR_RPC_ERROR:
			return -EPROTO;
	}

	return -EPROTO;
}
