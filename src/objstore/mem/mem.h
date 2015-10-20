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

#ifndef __NOMAD_OBJSTORE_MEM_H
#define __NOMAD_OBJSTORE_MEM_H

#include <sys/avl.h>

#include <nomad/mutex.h>
#include <nomad/atomic.h>

/* each <oid,ver> */
struct memobj {
	/* key */
	struct nobjhndl handle;

	/* value */
	struct nattr attrs;
	void *blob; /* used if the memobj is a file */

	avl_tree_t dentries; /* used if the memobj is a director */

	/* misc */
	avl_node_t node;
};

struct mem_dentry {
	const char *name;
	struct nobjhndl *handle;
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

#endif
