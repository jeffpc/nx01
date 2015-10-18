/*
 * Copyright (c) 2015 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __NOMAD_ATTR_H
#define __NOMAD_ATTR_H

#include <stdint.h>
#include <rpc/rpc.h>

/* object attributes */

#define NATTR_TMASK	0xf000 /* type mask */
#define NATTR_AMASK	0x0fff /* access bits mask */

#define NATTR_REG	0x0000 /* regular file */
#define NATTR_DIR	0x1000 /* directory */
#define NATTR_FIFO	0x2000 /* fifo */
#define NATTR_CHR	0x3000 /* character device */
#define NATTR_BLK	0x4000 /* block device */
#define NATTR_LNK	0x5000 /* symbolic link */
#define NATTR_SOCK	0x6000 /* socket */
#define NATTR_DOOR	0x7000 /* door */

#define NATTR_STICK	0x0200 /* sticky bit */
#define NATTR_SGID	0x0400 /* set-gid bit */
#define NATTR_SUID	0x0800 /* set-uid bit */

#define NATTR_ISREG(mode)	(((mode) & NATTR_TMASK) == NATTR_REG)
#define NATTR_ISDIR(mode)	(((mode) & NATTR_TMASK) == NATTR_DIR)
#define NATTR_ISFIFO(mode)	(((mode) & NATTR_TMASK) == NATTR_FIFO)
#define NATTR_ISCHR(mode)	(((mode) & NATTR_TMASK) == NATTR_CHR)
#define NATTR_ISBLK(mode)	(((mode) & NATTR_TMASK) == NATTR_BLK)
#define NATTR_ISLNK(mode)	(((mode) & NATTR_TMASK) == NATTR_LNK)
#define NATTR_ISSOCK(mode)	(((mode) & NATTR_TMASK) == NATTR_SOCK)
#define NATTR_ISDOOR(mode)	(((mode) & NATTR_TMASK) == NATTR_DOOR)

struct nattr {
	uint16_t _reserved;
	uint16_t mode; /* see NATTR_* macros above */
	uint32_t nlink;
	uint64_t size;
	uint64_t atime; /* access time */
	uint64_t btime; /* birth time */
	uint64_t ctime; /* change time */
	uint64_t mtime; /* modify time */
	/* XXX: owner */
	/* XXX: group */
};

extern bool_t xdr_nattr(XDR *xdrs, struct nattr *attr);

#endif
