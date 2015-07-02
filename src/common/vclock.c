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

#include <stdlib.h>
#include <string.h>

#include <nomad/types.h>
#include <nomad/error.h>

/*
 * Vector clocks
 *
 * A vector clock is a set of <node, sequence id> pairs.  Since it is a set,
 * each node may appear at most once.  There are a few implied behaviors:
 *
 * (1) if a node doesn't appear in the vector, it's sequence id is assumed
 *     to be zero.
 * (2) node zero is reserved.  Internally to this code, it can be used to
 *     indicate an unused entry.  However, a sequence id get of node zero
 *     will return zero.
 *
 * As far as this implementation of vector clocks is concerned, the ent[]
 * array is managed as follows:
 *
 * (1) nodes with zero sequence id are *not* stored.  This is important for
 *     the set function.  Whenever the new sequence id ends up equal to
 *     zero, the <node, seq> pair is removed from the vector.
 * (2) entries may not be used contiguously.
 */

struct nvclock *nvclock_alloc(void)
{
	return zalloc(sizeof(struct nvclock));
}

struct nvclock *nvclock_dup(struct nvclock *clock)
{
	struct nvclock *ret;

	ret = nvclock_alloc();
	if (!ret)
		return NULL;

	memcpy(ret, clock, sizeof(*ret));

	return ret;
}

void nvclock_free(struct nvclock *clock)
{
	free(clock);
}

static struct nvclockent *__get_ent(struct nvclock *clock, uint64_t node,
				    bool alloc)
{
	int i;

	if (!clock || !node)
		return ERR_PTR(EINVAL);

	for (i = 0; i < NVCLOCK_NUM_NODES; i++)
		if (clock->ent[i].node == node)
			return &clock->ent[i];

	if (!alloc)
		return ERR_PTR(ENOENT);

	/*
	 * We didn't find it; we're supposed to allocate an entry.
	 */

	for (i = 0; i < NVCLOCK_NUM_NODES; i++) {
		if (!clock->ent[i].node) {
			clock->ent[i].node = node;
			return &clock->ent[i];
		}
	}

	return ERR_PTR(ENOMEM);
}

/*
 * Get @clock's @node sequence id.
 */
uint64_t nvclock_get_node(struct nvclock *clock, uint64_t node)
{
	struct nvclockent *ent;

	ent = __get_ent(clock, node, false);

	return IS_ERR(ent) ? 0 : ent->seq;
}

uint64_t nvclock_get(struct nvclock *clock)
{
	return nvclock_get_node(clock, nomad_local_node_id());
}

/*
 * Remove @node from @clock.
 */
int nvclock_remove_node(struct nvclock *clock, uint64_t node)
{
	struct nvclockent *ent;

	if (!clock || !node)
		return EINVAL;

	ent = __get_ent(clock, node, false);
	if (IS_ERR(ent))
		return 0;

	ent->node = 0;
	ent->seq = 0;

	return 0;
}

int nvclock_remove(struct nvclock *clock)
{
	return nvclock_remove_node(clock, nomad_local_node_id());
}

/*
 * Set @clock's @node to @seq.
 */
int nvclock_set_node(struct nvclock *clock, uint64_t node, uint64_t seq)
{
	struct nvclockent *ent;

	if (!seq)
		return nvclock_remove_node(clock, node);

	ent = __get_ent(clock, node, true);
	if (IS_ERR(ent))
		return PTR_ERR(ent);

	ent->seq = seq;

	return 0;
}

int nvclock_set(struct nvclock *clock, uint64_t seq)
{
	return nvclock_set_node(clock, nomad_local_node_id(), seq);
}

/*
 * Increment @clock's @node by 1.
 */
int nvclock_inc_node(struct nvclock *clock, uint64_t node)
{
	struct nvclockent *ent;

	ent = __get_ent(clock, node, true);
	if (IS_ERR(ent))
		return PTR_ERR(ent);

	ent->seq++;

	return 0;
}

int nvclock_inc(struct nvclock *clock)
{
	return nvclock_inc_node(clock, nomad_local_node_id());
}

static int vc_cmp(const void *va, const void *vb)
{
	const struct nvclockent *a = va;
	const struct nvclockent *b = vb;

	if (a->node < b->node)
		return -1;
	if (a->node > b->node)
		return 1;
	return 0;
}

/*
 * Are all elements in @u <= than their corresponding elements in @v?
 */
static bool all_le(struct nvclock *u, int i, struct nvclock *v, int j)
{
	while ((i < NVCLOCK_NUM_NODES) && (j < NVCLOCK_NUM_NODES)) {
		struct nvclockent *a = &u->ent[i];
		struct nvclockent *b = &v->ent[j];


		if (a->node < b->node) {
			/*
			 * @v doesn't have this node, therefore it is
			 * assumed to be zero.  Since @u's element won't be
			 * zero (otherwise it'd be omitted), we are
			 * essentially asking this question:
			 *
			 * x <= 0, where x >= 1
			 *
			 * which is obviously false.
			 */
			return false;
		}

		if (a->node == b->node) {
			if (a->seq > b->seq)
				return false;

			/* advance both */
			i++;
			j++;
		} else {
			/* let @u catch up */
			i++;
		}
	}

	return true;
}

enum nvclockcmp nvclock_cmp(const struct nvclock *c1, const struct nvclock *c2)
{
	struct nvclock u, v;
	int i, j;

	u = *c1;
	v = *c2;

	qsort(u.ent, NVCLOCK_NUM_NODES, sizeof(struct nvclockent), vc_cmp);
	qsort(v.ent, NVCLOCK_NUM_NODES, sizeof(struct nvclockent), vc_cmp);

	/* find first non-zero node */
	for (i = 0; i < NVCLOCK_NUM_NODES; i++)
		if (u.ent[i].node)
			break;

	for (j = 0; j < NVCLOCK_NUM_NODES; j++)
		if (v.ent[j].node)
			break;

	/* handle some trivial cases (one or both vectors being empty */
	if (i == NVCLOCK_NUM_NODES && j == NVCLOCK_NUM_NODES)
		return NVC_EQ;
	if (i == NVCLOCK_NUM_NODES)
		return NVC_LT;
	if (j == NVCLOCK_NUM_NODES)
		return NVC_GT;

	/*
	 * We have two non-empty vectors.
	 */
	if (i == j) {
		bool all_eq;
		int tmp;

		/*
		 * Both vectors have the same number of elements so they
		 * have a chance of being equal
		 */
		all_eq = true;

		for (tmp = i; all_eq && (tmp < NVCLOCK_NUM_NODES); tmp++)
			all_eq = ((u.ent[tmp].node == v.ent[tmp].node) &&
				  (u.ent[tmp].seq == v.ent[tmp].seq));

		if (all_eq)
			return NVC_EQ;
	}

	if (all_le(&u, i, &v, j))
		return NVC_LT;

	if (all_le(&v, j, &u, i))
		return NVC_GT;

	return NVC_DIV;
}

int nvclock_cmp_total(const struct nvclock *c1, const struct nvclock *c2)
{
	switch (nvclock_cmp(c1, c2)) {
		case NVC_LT:
			return -1;
		case NVC_EQ:
			return 0;
		case NVC_GT:
			return 1;
		case NVC_DIV:
			/* this is the hard part...see below */
			break;
	}

	/*
	 * We have two clocks and neither is an ancestor of the other.
	 * Since we are after a total ordering, we need to decide whether we
	 * should return -1 or +1.  (We cannot return 0 since that'd imply
	 * that the two clocks are equivalent - and they definitely aren't.)
	 *
	 * XXX: describe algo
	 */

#warning "not yet implemented"
	// XXX
}
