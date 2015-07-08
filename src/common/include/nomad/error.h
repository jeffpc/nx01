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

#ifndef __NOMAD_ERROR_H
#define __NOMAD_ERROR_H

#include <stdint.h>
#include <errno.h>
#include <string.h>

#define MAX_ERRNO	1023

static inline int PTR_ERR(void *ptr)
{
	return -(intptr_t) ptr;
}

static inline void *ERR_PTR(int err)
{
	return (void *)(intptr_t) -err;
}

static inline int IS_ERR(void *ptr)
{
	intptr_t err = (intptr_t) ptr;

	return (err < 0) && (err >= -MAX_ERRNO);
}

extern void assfail(const char *, const char *, int);
extern void assfail3(const char *, uintmax_t, const char *, uintmax_t,
		     const char *, int);

#define VERIFY3P(l, op, r)						\
	do {								\
		uintptr_t lhs = (uintptr_t) (l);			\
		uintptr_t rhs = (uintptr_t) (r);			\
		if (!((lhs) op (rhs)))					\
			assfail3(#l " " #op " " #r, lhs, #op, rhs,	\
				 __FILE__, __LINE__);			\
	} while (0)

#define VERIFY3U(l, op, r)						\
	do {								\
		uint64_t lhs = (l);			\
		uint64_t rhs = (r);			\
		if (!((lhs) op (rhs)))					\
			assfail3(#l " " #op " " #r, lhs, #op, rhs,	\
				 __FILE__, __LINE__);			\
	} while (0)

#define VERIFY3S(l, op, r)						\
	do {								\
		int64_t lhs = (l);			\
		int64_t rhs = (r);			\
		if (!((lhs) op (rhs)))					\
			assfail3(#l " " #op " " #r, lhs, #op, rhs,	\
				 __FILE__, __LINE__);			\
	} while (0)

#define VERIFY(c)							\
	do {								\
		if (!(c))						\
			assfail(#c, __FILE__, __LINE__);		\
	} while (0)

#define VERIFY0(c)	VERIFY3U((c), ==, 0)

#define ASSERT3P(l, op, r)	VERIFY3P(l, op, r)
#define ASSERT3U(l, op, r)	VERIFY3U(l, op, r)
#define ASSERT3S(l, op, r)	VERIFY3S(l, op, r)
#define ASSERT0(c)		VERIFY0(c)
#define ASSERT(c)		VERIFY(c)

#endif
