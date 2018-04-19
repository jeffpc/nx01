/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * Copyright (c) 2015 Holly Sipek
 * Copyright (c) 2016 Steve Dougherty
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
#include <jeffpc/socksvc.h>

#include <nomad/types.h>
#include <nomad/objstore.h>
#include <nomad/init.h>

#include "cmds.h"
#include "ohandle.h"

#define CLIENT_DAEMON_PORT	2323

static void connection_acceptor(int fd, struct socksvc_stats *stats, void *arg)
{
	struct fsconn conn;

	conn.fd = fd;
	conn.vol = NULL;

	avl_create(&conn.open_handles, ohandle_cmp, sizeof(struct ohandle),
		   offsetof(struct ohandle, node));

	cmn_err(CE_DEBUG, "%s: fd = %d, arg = %p", __func__, fd, arg);

	if (process_handshake(&conn))
		while (process_connection(&conn))
			;

	ohandle_close_all(&conn);

	avl_destroy(&conn.open_handles);
}

int main(int argc, char **argv)
{
	struct objstore *vol;
	int ret;

	ret = ohandle_init();
	if (ret) {
		cmn_err(CE_CRIT, "failed to initialize ohandle subsystem: %s",
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

	vol = objstore_vol_create("myfiles");
	cmn_err(CE_DEBUG, "vol = %p", vol);

	if (IS_ERR(vol)) {
		ret = PTR_ERR(vol);
		cmn_err(CE_CRIT, "error: %s", xstrerror(ret));
		goto err_init;
	}

	ret = objstore_vdev_create(vol, "mem", "fauxpath");
	cmn_err(CE_DEBUG, "vdev create = %d", ret);

	if (ret) {
		cmn_err(CE_CRIT, "error: %s", xstrerror(ret));
		goto err_vol;
	}

	ret = socksvc(NULL, CLIENT_DAEMON_PORT, -1, connection_acceptor, NULL);

	cmn_err(CE_DEBUG, "socksvc() = %d (%s)", ret, xstrerror(ret));

	/* XXX: undo objstore_vdev_create() */

err_vol:
	/* XXX: undo objstore_vol_create() */

err_init:
	/* XXX: undo objstore_init() */

err:
	return ret;
}
