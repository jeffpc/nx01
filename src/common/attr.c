/*
 * Copyright (c) 2015 Joshua Kahn <josh@joshuak.net>
 * Copyright (c) 2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#define MAP_OR(i, o)		case i: mode |= o; break

static inline mode_t cvt_nmode(uint16_t nmode)
{
	mode_t mode;

	/* copy permission bits */
	mode = nmode & NATTR_AMASK;

	switch (nmode & NATTR_TMASK) {
		MAP_OR(NATTR_REG, S_IFREG);
		MAP_OR(NATTR_DIR, S_IFDIR);
		MAP_OR(NATTR_FIFO, S_IFIFO);
		MAP_OR(NATTR_CHR, S_IFCHR);
		MAP_OR(NATTR_BLK, S_IFBLK);
		MAP_OR(NATTR_LNK, S_IFLNK);
		MAP_OR(NATTR_SOCK, S_IFSOCK);
#ifdef HAVE_DOORS
		MAP_OR(NATTR_DOOR, S_IFDOOR);
#else
		case NATTR_DOOR:
			panic("unable to map NATTR_DOOR");
#endif
		default:
			panic("unknown nomad mode type: %o", nmode);
	}

	return mode;
}

static inline void cvt_ntime(struct timespec *s, const uint64_t t)
{
	s->tv_sec = t / 1000000000;
	s->tv_nsec = t % 1000000000;
}

void nattr_to_stat(const struct nattr *nattr, struct stat *stat)
{
	stat->st_dev = 0;
	stat->st_rdev = 0;
	stat->st_ino = 0;
	stat->st_mode = cvt_nmode(nattr->mode);
	stat->st_nlink = nattr->nlink;
	stat->st_uid = 0;
	stat->st_gid = 0;
	stat->st_size = nattr->size;
	cvt_ntime(&stat->st_atim, nattr->atime);
	cvt_ntime(&stat->st_ctim, nattr->ctime);
	cvt_ntime(&stat->st_mtim, nattr->mtime);
	stat->st_blksize = 4096;
	stat->st_blocks = (nattr->size + stat->st_blksize - 1) / stat->st_blksize;

	/*
	 * TODO: Illumos has a stat->st_fstype, should we zero it?  Linux
	 * does not have this field.
	 */
}

bool_t xdr_nattr(XDR *xdrs, struct nattr *attr)
{
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
