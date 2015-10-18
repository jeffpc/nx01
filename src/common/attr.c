/*
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

#include <nomad/attr.h>

bool_t xdr_nattr(XDR *xdrs, struct nattr *attr)
{
	if (!xdr_uint16_t(xdrs, &attr->_reserved))
		return FALSE;
	if (!xdr_uint16_t(xdrs, &attr->mode))
		return FALSE;
	if (!xdr_uint32_t(xdrs, &attr->nlink))
		return FALSE;
	if (!xdr_uint64_t(xdrs, &attr->size))
		return FALSE;
	if (!xdr_uint64_t(xdrs, &attr->atime))
		return FALSE;
	if (!xdr_uint64_t(xdrs, &attr->btime))
		return FALSE;
	if (!xdr_uint64_t(xdrs, &attr->ctime))
		return FALSE;
	if (!xdr_uint64_t(xdrs, &attr->mtime))
		return FALSE;
	return TRUE;
}
