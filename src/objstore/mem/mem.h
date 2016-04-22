/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * Copyright (c) 2015 Holly Sipek
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

#ifndef __NOMAD_OBJSTORE_MEM_H
#define __NOMAD_OBJSTORE_MEM_H

#include <sys/avl.h>
#include <jeffpc/atomic.h>
#include <jeffpc/refcnt.h>

#include <nomad/types.h>
#include <nomad/mutex.h>

/*
 * There is a handful of structures that keep track of everything.  At the
 * highest level, there is `struct memstore'.  It keeps track of the
 * volume's state.  Most importantly, it keeps track of all objects this
 * volume knows about on the `obj' AVL tree.  Each object is represented by
 * a `struct memobj'.
 *
 * Each object has a unique ID - the object ID (`struct noid').  This
 * uniquely identifies it not just within the volume, but also within the
 * volume group (thanks to the dataset ID within it).  There can be more
 * than one version of an object.  If there is more than one version of an
 * object, they should be divergent (nvclock_cmp() would return NVC_DIV)
 * since given non-divergent versions we would simply take the newer one
 * (NVC_GT) and discard the older (NVC_LT).  `struct memobj' keeps track of
 * all the versions of that object via the `versions' AVL tree.  Each
 * version of an object is represented by a `struct memver'.
 *
 * Each version has a vector clock.  Unlike the object ID, the clock may
 * change over time as the object is modified.  The only modifications
 * allowed are:
 *
 *  (1) nvclock_inc() due to a local modification
 *  (2) nvclock_set_node() due to a merge of two versions
 *
 * Aside from the vector clock, `struct memver' contains the "values"
 * associated with the object - i.e., the file attributes, the file blob (in
 * case of a file), the dentries (in case of a directory).
 */

struct memobj;

/* each version */
struct memver {
	/* key */
	struct nvclock *clock;

	/* value */
	/*
	 * Everything in the attrs structure is used for attribute storage
	 * except for:
	 *   - nlink: use nlink field in struct memobj
	 */
	struct nattr attrs;
	void *blob; /* used if the memobj is a file */
	avl_tree_t dentries; /* used if the memobj is a director */

	/* misc */
	struct memobj *obj;
	avl_node_t node;
};

/* each oid */
struct memobj {
	/* key */
	struct noid oid;

	/* value */
	avl_tree_t versions;   /* all versions */
	uint32_t nlink;	 /* file link count */

	/* misc */
	avl_node_t node;
	refcnt_t refcnt;
	pthread_mutex_t lock;
};

/* <name> -> <specific version of an obj> */
struct memdentry {
	/* key */
	const char *name;

	/* value */
	struct memobj *obj;

	/* misc */
	avl_node_t node;
};

/* the whole store */
struct memstore {
	avl_tree_t objs;
	struct memobj *root;

	uint32_t ds; /* our dataset id */
	atomic64_t next_oid_uniq; /* the next unique part of noid */

	pthread_mutex_t lock;
};

extern const struct obj_ops obj_ops;

extern struct memobj *newmemobj(struct memstore *ms, uint16_t mode);
extern void freememobj(struct memobj *obj);
extern struct memobj *findmemobj(struct memstore *store, const struct noid *oid);

REFCNT_INLINE_FXNS(struct memobj, memobj, refcnt, freememobj);

#endif
