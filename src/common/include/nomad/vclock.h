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

#ifndef __NOMAD_VCLOCK_H
#define __NOMAD_VCLOCK_H

#include <stdint.h>
#include <rpc/rpc.h>

/* version vector */
#define NVCLOCK_NUM_NODES	16 /* ought to be enough for everyone */

enum nvclockcmp {
	NVC_LT = -1,
	NVC_EQ = 0,
	NVC_GT = 1,
	NVC_DIV = 2,
};

struct nvclockent {
	uint64_t node;
	uint64_t seq;
};

struct nvclock {
	struct nvclockent ent[NVCLOCK_NUM_NODES];
};

extern struct nvclock *nvclock_alloc(void);
extern struct nvclock *nvclock_dup(const struct nvclock *clock);
extern void nvclock_free(struct nvclock *clock);
extern enum nvclockcmp nvclock_cmp(const struct nvclock *c1,
				   const struct nvclock *c2);
extern int nvclock_cmp_total(const struct nvclock *c1,
			     const struct nvclock *c2);
extern uint64_t nvclock_get_node(struct nvclock *clock, uint64_t node);
extern int nvclock_remove_node(struct nvclock *clock, uint64_t node);
extern int nvclock_set_node(struct nvclock *clock, uint64_t node,
			    uint64_t seq);
extern int nvclock_inc_node(struct nvclock *clock, uint64_t node);
extern uint64_t nvclock_get(struct nvclock *clock);
extern int nvclock_remove(struct nvclock *clock);
extern int nvclock_set(struct nvclock *clock, uint64_t seq);
extern int nvclock_inc(struct nvclock *clock);
extern bool nvclock_is_null(const struct nvclock *clock);

extern bool_t xdr_nvclock(XDR *xdrs, struct nvclock **clock);

#endif
