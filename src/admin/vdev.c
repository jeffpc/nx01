/*
 * Copyright (c) 2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include "admin.h"

/* import and create are *very* similar */
static int __vdev_import(int argc, char **argv, bool create)
{
	char out[XUUID_PRINTABLE_STRING_LENGTH];
	struct xuuid uuid;
	const char *type;
	const char *path;
	int opt;
	int ret;

	type = NULL;
	while ((opt = getopt(argc, argv, "+t:")) != -1) {
		switch (opt) {
			case 't':
				if (type)
					return PRINT_USAGE;

				type = optarg;
				break;
			default:
				return PRINT_USAGE;
		}
	}

	if (!type)
		return PRINT_USAGE;

	/* extra args other than the path? */
	if ((optind + 1) != argc)
		return PRINT_USAGE;

	path = argv[optind];

	if ((ret = connect_to_server()))
		return ret;

	ret = fscall_vdev_import(&state, type, path, create, &uuid);

	disconnect_from_server();

	if (ret) {
		fprintf(stderr, "Error: %s\n", xstrerror(nerr_to_errno(ret)));
		return 1;
	}

	xuuid_unparse(&uuid, out);
	printf("%s\n", out);
	return 0;
}

void cmd_vdev_create_usage(void)
{
	fprintf(stderr, "-t <type> <path>\n\n");

	print_option("-t <type>", "type of vdev to create (e.g., 'posix')");
	print_option("<path>", "type-specific path");
}

int cmd_vdev_create(int argc, char **argv)
{
	return __vdev_import(argc, argv, true);
}

void cmd_vdev_import_usage(void)
{
	fprintf(stderr, "-t <type> <path>\n\n");

	print_option("-t <type>", "type of vdev to import (e.g., 'posix')");
	print_option("<path>", "type-specific path");
}

int cmd_vdev_import(int argc, char **argv)
{
	return __vdev_import(argc, argv, false);
}
