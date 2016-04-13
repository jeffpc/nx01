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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <jeffpc/error.h>

#include <nomad/types.h>
#include <nomad/objstore.h>
#include <nomad/connsvc.h>
#include <nomad/init.h>

#include "cmds.h"

#define CLIENT_DAEMON_PORT	2323

static void connection_acceptor(int fd, void *arg)
{
	struct fsconn conn;

	conn.fd = fd;
	conn.vg = NULL;
	cmn_err(CE_DEBUG, "%s: fd = %d, arg = %p", __func__, fd, arg);

	while (process_connection(&conn))
		;
}

int main(int argc, char **argv)
{
	struct objstore *vg;
	int ret;

	ret = common_init();
	if (ret) {
		cmn_err(CE_CRIT, "common_init() = %d (%s)", ret,
			xstrerror(ret));
		goto err;
	}

	ret = objstore_init();
	if (ret) {
		cmn_err(CE_CRIT, "objstore_init() = %d (%s)", ret,
			xstrerror(ret));

		if (ret == ENOENT)
			cmn_err(CE_CRIT, "Did you set LD_LIBRARY_PATH?");

		goto err;
	}

	vg = objstore_vg_create("myfiles");
	cmn_err(CE_DEBUG, "vg = %p", vg);

	if (IS_ERR(vg)) {
		ret = PTR_ERR(vg);
		cmn_err(CE_CRIT, "error: %s", xstrerror(ret));
		goto err_init;
	}

	ret = objstore_vol_create(vg, "fauxpath", OS_MODE_STORE);
	cmn_err(CE_DEBUG, "vol create = %d", ret);

	if (ret) {
		cmn_err(CE_CRIT, "error: %s", xstrerror(ret));
		goto err_vg;
	}

	ret = connsvc(NULL, CLIENT_DAEMON_PORT, connection_acceptor, NULL);

	cmn_err(CE_DEBUG, "connsvc() = %d (%s)", ret, xstrerror(ret));

	/* XXX: undo objstore_vol_create() */

err_vg:
	/* XXX: undo objstore_vg_create() */

err_init:
	/* XXX: undo objstore_init() */

err:
	return ret;
}
