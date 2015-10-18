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
#include <stdlib.h>
#include <unistd.h>

#include <nomad/error.h>
#include <nomad/types.h>
#include <nomad/objstore.h>
#include <nomad/connsvc.h>

#include "cmds.h"

#define CLIENT_DAEMON_PORT	2323

static void connection_acceptor(int fd, void *arg)
{
	printf("%s: fd = %d, arg = %p\n", __func__, fd, arg);

	while (process_connection(fd))
		;
}

int main(int argc, char **argv)
{
	struct objstore *vg;
	struct objstore_vol *vol;
	int ret;

	/*
	 * TODO:
	 * The local node id should be stored persistently in a config file
	 * of some sort.  The initial value for the node id should be
	 * generated randomly.
	 */
	nomad_set_local_node_id(0xabba);

	ret = objstore_init();
	if (ret) {
		fprintf(stderr, "objstore_init() = %d (%s)\n", ret,
			strerror(ret));

		if (ret == ENOENT)
			fprintf(stderr, "Did you set LD_LIBRARY_PATH?\n");

		goto err;
	}

	vg = objstore_vg_create("myfiles", OS_VG_SIMPLE);
	fprintf(stderr, "vg = %p\n", vg);

	if (IS_ERR(vg)) {
		ret = PTR_ERR(vg);
		fprintf(stderr, "error: %s\n", strerror(ret));
		goto err_init;
	}

	vol = objstore_vol_create(vg, "fauxpath", OS_MODE_STORE);
	fprintf(stderr, "vol = %p\n", vol);

	if (IS_ERR(vol)) {
		ret = PTR_ERR(vol);
		fprintf(stderr, "error: %s\n", strerror(ret));
		goto err_vg;
	}

	ret = connsvc(NULL, CLIENT_DAEMON_PORT, connection_acceptor, NULL);

	fprintf(stderr, "connsvc() = %d (%s)\n", ret, strerror(ret));

	/* XXX: undo objstore_vol_create() */

err_vg:
	/* XXX: undo objstore_vg_create() */

err_init:
	/* XXX: undo objstore_init() */

err:
	return ret;
}
