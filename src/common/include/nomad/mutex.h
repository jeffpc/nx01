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

#ifndef __NOMAD_MUTEX_H
#define __NOMAD_MUTEX_H

#include <stdbool.h>
#include <pthread.h>

extern void mxinit(pthread_mutex_t *m);
extern void mxdestroy(pthread_mutex_t *m);
extern void mxlock(pthread_mutex_t *m);
extern void mxunlock(pthread_mutex_t *m);

extern void rwinit(pthread_rwlock_t *l);
extern void rwdestroy(pthread_rwlock_t *l);
extern void rwlock(pthread_rwlock_t *l, bool wr);
extern void rwunlock(pthread_rwlock_t *l);

extern void condinit(pthread_cond_t *c);
extern void conddestroy(pthread_cond_t *c);
extern void condwait(pthread_cond_t *c, pthread_mutex_t *m);
extern void condsig(pthread_cond_t *c);
extern void condbcast(pthread_cond_t *c);

#endif
