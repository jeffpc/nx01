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
 *     the set/inc functions.  Whenever the new sequence id ends up equal to
 *     zero, the <node, seq> pair is removed from the vector.
 * (2) entries may not be used contiguously.
 */

struct nvclock *nvclock_alloc(void)
{
	return malloc(sizeof(struct nvclock));
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

/*
 * Set @clock's @node to @seq.
 */
int nvclock_set_node(struct nvclock *clock, uint64_t node, uint64_t seq)
{
	struct nvclockent *ent;

	ent = __get_ent(clock, node, true);
	if (IS_ERR(ent))
		return PTR_ERR(ent);

	ent->seq = seq;

	return 0;
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
