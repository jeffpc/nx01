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

#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

#include <nomad/config.h>
#include <nomad/error.h>

#ifndef HAVE_ASSFAIL
void assfail(const char *assertion, const char *file, int line)
{
	cmn_err(CE_CRIT, "Assertion failed: %s", line);

	__assert(assertion, file, line);
}
#endif

#ifndef HAVE_ASSFAIL3
void assfail3(const char *assertion, uintmax_t lhs, const char *op,
	      uintmax_t rhs, const char *file, int line)
{
	char msg[512];

	snprintf(msg, sizeof(msg), "%s (%#"PRIx64" %s %#"PRIx64")",
		 assertion, lhs, op, rhs);

	cmn_err(CE_CRIT, "Assertion failed: %s", msg);

	__assert(msg, file, line);
}
#endif
