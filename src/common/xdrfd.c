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

#include <unistd.h>

#include <nomad/error.h>
#include <nomad/rpc.h>

static void invalid_op(void)
{
	ASSERT(0);
}

static ssize_t safe_read(int fd, void *buf, size_t nbyte)
{
	char *ptr = buf;
	size_t total;
	ssize_t ret;

	total = 0;

	while (nbyte) {
		ret = read(fd, ptr, nbyte);
		if (ret < 0)
			return -1;

		if (ret == 0)
			break;

		nbyte -= ret;
		total += ret;
		ptr   += ret;
	}

	return total;
}

static ssize_t safe_write(int fd, void *buf, size_t nbyte)
{
	const char *ptr = buf;
	size_t total;
	ssize_t ret;

	total = 0;

	while (nbyte) {
		ret = write(fd, ptr, nbyte);
		if (ret < 0)
			return -1;

		if (ret == 0)
			break;

		nbyte -= ret;
		total += ret;
		ptr   += ret;
	}

	return total;
}

static bool_t xdrfd_getbytes(XDR *xdr, caddr_t addr, int len)
{
	int fd = xdr->x_handy;

	if (safe_read(fd, addr, len) != len)
		return FALSE;
	return TRUE;
}

static bool_t xdrfd_putbytes(XDR *xdr, caddr_t addr, int len)
{
	int fd = xdr->x_handy;

	if (safe_write(fd, addr, len) != len)
		return FALSE;
	return TRUE;
}

static void xdrfd_destroy(XDR *xdr)
{
	fsync(xdr->x_handy);
}

static bool_t xdrfd_getint32(XDR *xdr, int32_t *p)
{
	int fd = xdr->x_handy;
	int32_t buf = 0;

	if (safe_read(fd, &buf, sizeof(buf)) != sizeof(buf))
		return FALSE;

	*p = ntohl(buf);
	return TRUE;
}

static bool_t xdrfd_putint32(XDR *xdr, int32_t *p)
{
	int fd = xdr->x_handy;
	int32_t buf;

	buf = htonl(*p);

	if (safe_write(fd, &buf, sizeof(buf)) != sizeof(buf))
		return FALSE;

	return TRUE;
}

static bool_t xdrfd_getlong(XDR *xdrs, long *p)
{
        int32_t tmp;

        if (!xdrfd_getint32(xdrs, &tmp))
                return FALSE;

        *p = (long) tmp;
        return TRUE;
}

static bool_t xdrfd_putlong(XDR *xdrs, long *p)
{
        int32_t tmp = *p;

        return xdrfd_putint32(xdrs, &tmp);
}

static const struct xdr_ops ops = {
	.x_getbytes = xdrfd_getbytes,
	.x_putbytes = xdrfd_putbytes,
	.x_getpostn = (void*) invalid_op,
	.x_setpostn = (void*) invalid_op,
	.x_inline   = (void*) invalid_op,
	.x_destroy  = xdrfd_destroy,
#ifdef _LP64
	.x_getint32 = xdrfd_getint32,
	.x_putint32 = xdrfd_putint32,
#endif
	.x_getlong  = xdrfd_getlong,
	.x_putlong  = xdrfd_putlong,
};

void xdrfd_create(XDR *xdr, int fd, enum xdr_op op)
{
	xdr->x_op = op;
	xdr->x_ops = (struct xdr_ops *) &ops;
	xdr->x_handy = fd;
}
