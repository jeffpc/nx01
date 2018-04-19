/*
 * Copyright (c) 2017-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

#include <jeffpc/error.h>

#include <nomad/types.h>
#include <nomad/objstore.h>
#include <nomad/init.h>

static int usage(const char *prog)
{
	fprintf(stderr, "Usage: %s <type> <path>\n", prog);

	return 1;
}

static int do_format(const char *type, const char *path)
{
	FIXME("not yet implemented");
	return -ENOTSUP;
}

int main(int argc, char **argv)
{
	int ret;

	if (argc != 3)
		return usage(argv[0]);

	ret = objstore_init();
	if (ret) {
		cmn_err(CE_CRIT, "objstore_init() = %d (%s)", ret,
			xstrerror(ret));

		if (ret == -ENOENT)
			cmn_err(CE_CRIT, "Did you set LD_LIBRARY_PATH?");

		goto err;
	}

	ret = do_format(argv[1], argv[2]);
	if (ret)
		cmn_err(CE_CRIT, "Failed to create vdev: %s", xstrerror(ret));

	/* XXX: undo objstore_init() */

err:
	return ret;
}
