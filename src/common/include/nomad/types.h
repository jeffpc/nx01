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

#ifndef __NOMAD_TYPES_H
#define __NOMAD_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/types.h>
#include <rpc/rpc.h>

#include <nomad/attr.h>
#include <nomad/vclock.h>
#include <nomad/malloc.h>

#include <jeffpc/uuid.h>
#include <jeffpc/types.h>

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

/* object id */
struct noid {
	uint32_t ds;		/* dataset id */
	uint32_t _reserved;	/* must be zero */
	uint64_t uniq;		/* dataset-local id */
};

extern int nomad_set_local_node_id(uint64_t newid);
extern uint64_t nomad_local_node_id(void);

extern void noid_set(struct noid *n1, uint32_t ds, uint64_t uniq);
extern int noid_cmp(const struct noid *n1, const struct noid *n2);
extern bool_t xdr_noid(XDR *xdrs, struct noid *oid);

#endif
