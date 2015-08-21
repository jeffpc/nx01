/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2011 Nexenta Systems, Inc.  All rights reserved.
 * Copyright 2012 Garrett D'Amore <garrett@damore.org>.  All rights reserved.
 * Copyright (c) 2014 by Delphix. All rights reserved.
 * Copyright (c) 2015 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 */

#include <unistd.h>

#include <nomad/types.h>
#include <nomad/error.h>
#include <nomad/mutex.h>
#include <nomad/taskq.h>

/* Maximum percentage allowed for TASKQ_THREADS_CPU_PCT */
static int taskq_cpupct_max_percent = 1000;

int taskq_now;

#define	TASKQ_ACTIVE	0x00010000
#define	TASKQ_NAMELEN	31

struct taskq {
	char		tq_name[TASKQ_NAMELEN + 1];
	pthread_mutex_t	tq_lock;
	pthread_rwlock_t tq_threadlock;
	pthread_cond_t	tq_dispatch_cv;
	pthread_cond_t	tq_wait_cv;
	pthread_t	*tq_threadlist;
	int		tq_flags;
	int		tq_active;
	int		tq_nthreads;
	int		tq_nalloc;
	int		tq_minalloc;
	int		tq_maxalloc;
	pthread_cond_t	tq_maxalloc_cv;
	int		tq_maxalloc_wait;
	taskq_ent_t	*tq_freelist;
	taskq_ent_t	tq_task;
};

static taskq_ent_t *
task_alloc(taskq_t *tq, int tqflags)
{
	taskq_ent_t *t;
	struct timespec ts;
	int err;

again:	if ((t = tq->tq_freelist) != NULL && tq->tq_nalloc >= tq->tq_minalloc) {
		tq->tq_freelist = t->tqent_next;
	} else {
		if (tq->tq_nalloc >= tq->tq_maxalloc) {
			if (!(tqflags & UMEM_NOFAIL))
				return (NULL);

			/*
			 * We don't want to exceed tq_maxalloc, but we can't
			 * wait for other tasks to complete (and thus free up
			 * task structures) without risking deadlock with
			 * the caller.  So, we just delay for one second
			 * to throttle the allocation rate. If we have tasks
			 * complete before one second timeout expires then
			 * taskq_ent_free will signal us and we will
			 * immediately retry the allocation.
			 */
			tq->tq_maxalloc_wait++;

			ts.tv_sec = 1;
			ts.tv_nsec = 0;
			err = condreltimedwait(&tq->tq_maxalloc_cv,
			    &tq->tq_lock, &ts);

			tq->tq_maxalloc_wait--;
			if (err == 0)
				goto again;		/* signaled */
		}
		mxunlock(&tq->tq_lock);

		t = umem_alloc(sizeof (taskq_ent_t), tqflags);

		mxlock(&tq->tq_lock);
		if (t != NULL)
			tq->tq_nalloc++;
	}
	return (t);
}

static void
task_free(taskq_t *tq, taskq_ent_t *t)
{
	if (tq->tq_nalloc <= tq->tq_minalloc) {
		t->tqent_next = tq->tq_freelist;
		tq->tq_freelist = t;
	} else {
		tq->tq_nalloc--;
		mxunlock(&tq->tq_lock);
		umem_free(t, sizeof (taskq_ent_t));
		mxlock(&tq->tq_lock);
	}

	if (tq->tq_maxalloc_wait)
		condsig(&tq->tq_maxalloc_cv);
}

taskqid_t
taskq_dispatch(taskq_t *tq, task_func_t func, void *arg, unsigned int tqflags)
{
	taskq_ent_t *t;

	if (taskq_now) {
		func(arg);
		return (1);
	}

	mxlock(&tq->tq_lock);
	ASSERT(tq->tq_flags & TASKQ_ACTIVE);
	if ((t = task_alloc(tq, tqflags)) == NULL) {
		mxunlock(&tq->tq_lock);
		return (0);
	}
	if (tqflags & TQ_FRONT) {
		t->tqent_next = tq->tq_task.tqent_next;
		t->tqent_prev = &tq->tq_task;
	} else {
		t->tqent_next = &tq->tq_task;
		t->tqent_prev = tq->tq_task.tqent_prev;
	}
	t->tqent_next->tqent_prev = t;
	t->tqent_prev->tqent_next = t;
	t->tqent_func = func;
	t->tqent_arg = arg;
	t->tqent_flags = 0;
	condsig(&tq->tq_dispatch_cv);
	mxunlock(&tq->tq_lock);
	return (1);
}

void
taskq_dispatch_ent(taskq_t *tq, task_func_t func, void *arg, unsigned int flags,
    taskq_ent_t *t)
{
	ASSERT(func != NULL);
	ASSERT(!(tq->tq_flags & TASKQ_DYNAMIC));

	/*
	 * Mark it as a prealloc'd task.  This is important
	 * to ensure that we don't free it later.
	 */
	t->tqent_flags |= TQENT_FLAG_PREALLOC;
	/*
	 * Enqueue the task to the underlying queue.
	 */
	mxlock(&tq->tq_lock);

	if (flags & TQ_FRONT) {
		t->tqent_next = tq->tq_task.tqent_next;
		t->tqent_prev = &tq->tq_task;
	} else {
		t->tqent_next = &tq->tq_task;
		t->tqent_prev = tq->tq_task.tqent_prev;
	}
	t->tqent_next->tqent_prev = t;
	t->tqent_prev->tqent_next = t;
	t->tqent_func = func;
	t->tqent_arg = arg;
	condsig(&tq->tq_dispatch_cv);
	mxunlock(&tq->tq_lock);
}

void
taskq_wait(taskq_t *tq)
{
	mxlock(&tq->tq_lock);
	while (tq->tq_task.tqent_next != &tq->tq_task || tq->tq_active != 0)
		condwait(&tq->tq_wait_cv, &tq->tq_lock);
	mxunlock(&tq->tq_lock);
}

static void *
taskq_thread(void *arg)
{
	taskq_t *tq = arg;
	taskq_ent_t *t;
	bool prealloc;

	mxlock(&tq->tq_lock);
	while (tq->tq_flags & TASKQ_ACTIVE) {
		if ((t = tq->tq_task.tqent_next) == &tq->tq_task) {
			if (--tq->tq_active == 0)
				condbcast(&tq->tq_wait_cv);
			condwait(&tq->tq_dispatch_cv, &tq->tq_lock);
			tq->tq_active++;
			continue;
		}
		t->tqent_prev->tqent_next = t->tqent_next;
		t->tqent_next->tqent_prev = t->tqent_prev;
		t->tqent_next = NULL;
		t->tqent_prev = NULL;
		prealloc = t->tqent_flags & TQENT_FLAG_PREALLOC;
		mxunlock(&tq->tq_lock);

		rwlock(&tq->tq_threadlock, false);
		t->tqent_func(t->tqent_arg);
		rwunlock(&tq->tq_threadlock);

		mxlock(&tq->tq_lock);
		if (!prealloc)
			task_free(tq, t);
	}
	tq->tq_nthreads--;
	condbcast(&tq->tq_wait_cv);
	mxunlock(&tq->tq_lock);
	return (NULL);
}

/*ARGSUSED*/
taskq_t *
taskq_create(const char *name, int nthreads,
	int minalloc, int maxalloc, unsigned int flags)
{
	taskq_t *tq;
	int t;

	tq = zalloc(sizeof(taskq_t));
	if (!tq)
		return NULL;

	if (flags & TASKQ_THREADS_CPU_PCT) {
		int pct;
		ASSERT3S(nthreads, >=, 0);
		ASSERT3S(nthreads, <=, taskq_cpupct_max_percent);
		pct = MIN(nthreads, taskq_cpupct_max_percent);
		pct = MAX(pct, 0);

		nthreads = (sysconf(_SC_NPROCESSORS_ONLN) * pct) / 100;
		nthreads = MAX(nthreads, 1);	/* need at least 1 thread */
	} else {
		ASSERT3S(nthreads, >=, 1);
	}

	rwinit(&tq->tq_threadlock);
	mxinit(&tq->tq_lock);
	condinit(&tq->tq_dispatch_cv);
	condinit(&tq->tq_wait_cv);
	condinit(&tq->tq_maxalloc_cv);
	(void) strncpy(tq->tq_name, name, TASKQ_NAMELEN + 1);
	tq->tq_flags = flags | TASKQ_ACTIVE;
	tq->tq_active = nthreads;
	tq->tq_nthreads = nthreads;
	tq->tq_minalloc = minalloc;
	tq->tq_maxalloc = maxalloc;
	tq->tq_task.tqent_next = &tq->tq_task;
	tq->tq_task.tqent_prev = &tq->tq_task;
	tq->tq_threadlist =
	    umem_alloc(nthreads * sizeof (pthread_t), UMEM_NOFAIL);

	if (flags & TASKQ_PREPOPULATE) {
		mxlock(&tq->tq_lock);
		while (minalloc-- > 0)
			task_free(tq, task_alloc(tq, UMEM_NOFAIL));
		mxunlock(&tq->tq_lock);
	}

	for (t = 0; t < nthreads; t++)
		pthread_create(&tq->tq_threadlist[t], NULL, taskq_thread, tq);

	return (tq);
}

void
taskq_destroy(taskq_t *tq)
{
	int t;
	int nthreads = tq->tq_nthreads;

	taskq_wait(tq);

	mxlock(&tq->tq_lock);

	tq->tq_flags &= ~TASKQ_ACTIVE;
	condbcast(&tq->tq_dispatch_cv);

	while (tq->tq_nthreads != 0)
		condwait(&tq->tq_wait_cv, &tq->tq_lock);

	tq->tq_minalloc = 0;
	while (tq->tq_nalloc != 0) {
		ASSERT(tq->tq_freelist != NULL);
		task_free(tq, task_alloc(tq, UMEM_NOFAIL));
	}

	mxunlock(&tq->tq_lock);

	for (t = 0; t < nthreads; t++)
		pthread_join(tq->tq_threadlist[t], NULL);

	umem_free(tq->tq_threadlist, nthreads * sizeof (pthread_t));

	rwdestroy(&tq->tq_threadlock);
	mxdestroy(&tq->tq_lock);
	conddestroy(&tq->tq_dispatch_cv);
	conddestroy(&tq->tq_wait_cv);
	conddestroy(&tq->tq_maxalloc_cv);

	umem_free(tq, sizeof (taskq_t));
}

int
taskq_member(taskq_t *tq, void *t)
{
	int i;

	if (taskq_now)
		return (1);

	for (i = 0; i < tq->tq_nthreads; i++)
		if (tq->tq_threadlist[i] == (pthread_t)(uintptr_t)t)
			return (1);

	return (0);
}
