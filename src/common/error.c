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

#include <nomad/error.h>
#include <nomad/rpc.h>

#define MAP_ERRNO(errno)			\
	case errno: return NERR_##errno

int errno_to_nerr(int e)
{
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
		MAP_ERRNO(ENXIO);
		MAP_ERRNO(EPERM);
		MAP_ERRNO(EPROTO);
		MAP_ERRNO(ESRCH);
		MAP_ERRNO(EXDEV);
		case 0:
			return NERR_SUCCESS;
	}

	return NERR_UNKNOWN_ERROR;
}
