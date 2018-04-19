/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/error.h>

#include <nomad/types.h>

int noid_cmp(const struct noid *n1, const struct noid *n2)
{
	int ret;

	ret = xuuid_compare(&n1->vol, &n2->vol);
	if (ret < 0)
		return -1;
	if (ret > 0)
		return 1;

	if (n1->uniq < n2->uniq)
		return -1;
	if (n1->uniq > n2->uniq)
		return 1;

	return 0;
}

void noid_set(struct noid *oid, const struct xuuid *vol, uint64_t uniq)
{
	oid->vol = *vol;
	oid->uniq = uniq;
}

bool_t xdr_noid(XDR *xdrs, struct noid *oid)
{
	if (!xdr_xuuid(xdrs, &oid->vol))
		return FALSE;
	if (!xdr_uint64_t(xdrs, &oid->uniq))
		return FALSE;
	return TRUE;
}
