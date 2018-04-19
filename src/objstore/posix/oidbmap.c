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
#include <jeffpc/io.h>

#include "posix.h"

#define MAX_OID_UNIQ	(1ull << 32)
#define OID_BMAP_SIZE	(MAX_OID_UNIQ / 8)
#define OID_BMAP_OFFSET	4096

int oidbmap_create(struct posixvdev *pv)
{
	int ret;

	ret = xftruncate(pv->vdevfd, OID_BMAP_SIZE + OID_BMAP_OFFSET);
	if (ret)
		return ret;

	return oidbmap_set(pv, 0); /* reserve 0 - it is illegal */
}

int oidbmap_set(struct posixvdev *pv, uint64_t uniq)
{
	const off_t off = (uniq / 8) + OID_BMAP_OFFSET;
	const uint8_t bit = 1 << (uniq % 8);
	uint8_t tmp;
	int ret;

	/* TODO: locking */

	ret = xpread(pv->vdevfd, &tmp, sizeof(tmp), off);
	if (ret)
		return ret;

	tmp |= bit;

	return xpwrite(pv->vdevfd, &tmp, sizeof(tmp), off);
}

int oidbmap_get_new(struct posixvdev *pv, uint64_t *new)
{
	off_t off;
	int ret;

	/*
	 * FIXME: This is a terrible O(n) algorithm.  We can certainly do
	 * better than this.
	 */

	for (off = 0; off < OID_BMAP_SIZE; off++) {
		off_t file_off = off + OID_BMAP_SIZE;
		uint8_t tmp;
		int bitno;

		ret = xpread(pv->vdevfd, &tmp, sizeof(tmp), file_off);
		if (ret)
			return ret;

		/* all full */
		if (tmp == 0xff)
			continue;

		for (bitno = 0; bitno < 8; bitno++)
			if (!((1 << bitno) & tmp))
				break;

		tmp |= 1 << bitno;

		ret = xpwrite(pv->vdevfd, &tmp, sizeof(tmp), file_off);
		if (ret)
			return ret;

		*new = (off * 8) + bitno;

		return 0;
	}

	return -ENOSPC;
}

int oidbmap_put(struct posixvdev *pv, uint64_t uniq)
{
	const off_t off = (uniq / 8) + OID_BMAP_OFFSET;
	const uint8_t bit = 1 << (uniq % 8);
	uint8_t tmp;
	int ret;

	/* TODO: locking */

	ret = xpread(pv->vdevfd, &tmp, sizeof(tmp), off);
	if (ret)
		return ret;

	tmp &= ~bit;

	return xpwrite(pv->vdevfd, &tmp, sizeof(tmp), off);
}
