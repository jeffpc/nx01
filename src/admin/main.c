/*
 * Copyright (c) 2016-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/error.h>

#include <nomad/init.h>
#include <nomad/types.h>

const char *prog;

static int not_implemented(int argc, char **argv)
{
	fprintf(stderr, "'%s' command is not yet implemented\n",
		argv[1]);

	return 99;
}

static int cmd_host_id(int argc, char **argv)
{
	printf("%#016"PRIx64"\n", nomad_local_node_id());

	return 0;
}

static struct cmd {
	const char *name;
	int (*fxn)(int argc, char **argv);
} cmdtbl[] = {
	{ .name = "conn-add",    .fxn = not_implemented, },
	{ .name = "conn-list",   .fxn = not_implemented, },
	{ .name = "host-id",     .fxn = cmd_host_id, },
	{ .name = "pool-create", .fxn = not_implemented, },
	{ .name = "pool-import", .fxn = not_implemented, },
	{ .name = "pool-list",   .fxn = not_implemented, },
	{ .name = "vdev-create", .fxn = not_implemented, },
	{ .name = "vdev-list",   .fxn = not_implemented, },
};

static void usage(char *msg)
{
	int i;

	if (msg)
		fprintf(stderr, "%s\n\n", msg);

	fprintf(stderr, "Usage: %s <command>\n\n", prog);
	fprintf(stderr, "Available commands:\n");

	for (i = 0; i < ARRAY_LEN(cmdtbl); i++)
		fprintf(stderr, "\t%-16s\n", cmdtbl[i].name);

	exit(1);
}

int main(int argc, char **argv)
{
	int i;

	prog = argv[0];

	if (argc < 2)
		usage(NULL);

	for (i = 0; i < ARRAY_LEN(cmdtbl); i++)
		if (!strcmp(argv[1], cmdtbl[i].name))
			return cmdtbl[i].fxn(argc, argv);

	usage("unknown command");
}
