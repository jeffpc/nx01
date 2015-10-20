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

#include <nomad/types.h>
#include <nomad/error.h>

int noid_cmp(const struct noid *n1, const struct noid *n2)
{
	if (n1->ds < n2->ds)
		return -1;
	if (n1->ds > n2->ds)
		return 1;

	if (n1->uniq < n2->uniq)
		return -1;
	if (n1->uniq > n2->uniq)
		return 1;

	return 0;
}

void noid_set(struct noid *oid, uint32_t ds, uint64_t uniq)
{
	oid->ds = ds;
	oid->uniq = uniq;
	oid->_reserved = 0;
}

bool_t xdr_noid(XDR *xdrs, struct noid *oid)
{
	if (!xdr_uint32_t(xdrs, &oid->ds))
		return FALSE;
	if (!xdr_uint64_t(xdrs, &oid->uniq))
		return FALSE;
	return TRUE;
}

int nobjhndl_cpy(struct nobjhndl *dst, const struct nobjhndl *src)
{
	if (!dst || !src)
		return EINVAL;

	dst->oid = src->oid;
	dst->clock = src->clock;

	if (!src->clock)
		return 0;

	dst->clock = nvclock_dup(src->clock);
	return dst->clock ? 0 : ENOMEM;
}

bool_t xdr_nobjhndl(XDR *xdrs, struct nobjhndl *hndl)
{
	if (!xdr_noid(xdrs, &hndl->oid))
		return FALSE;
	if (!xdr_nvclock(xdrs, &hndl->clock))
		return FALSE;
	return TRUE;
}
