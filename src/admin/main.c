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

#include "admin.h"

#define DEFAULT_HOSTNAME	"localhost"
#define DEFAULT_PORT		2323

const char *prog;
static const char *hostname = DEFAULT_HOSTNAME;
static uint16_t port = DEFAULT_PORT;

static int not_implemented(int argc, char **argv)
{
	return PRINT_USAGE;

}

static int cmd_host_id(int argc, char **argv)
{
	printf("%#016"PRIx64"\n", nomad_local_node_id());

	return 0;
}

static struct cmd {
	const char *name;
	int (*fxn)(int argc, char **argv);
	const char *summary;
} cmdtbl[] = {
	{ "conn-add",    not_implemented, "add a new connection" },
	{ "conn-list",   not_implemented, "list existing connections" },
	{ "host-id",     cmd_host_id, "display host ID" },
	{ "vdev-import", not_implemented, "import a previously created vdev" },
	{ "vdev-list",   not_implemented, "list currently imported vdevs" },
	{ "vol-create",  not_implemented, "create a new volume" },
	{ "vol-import",  not_implemented, "import a previously created volume" },
	{ "vol-list",    not_implemented, "list currently imported volumes" },
};

__attribute__ ((format (printf, 2, 3)))
void print_option(const char *name, const char *descr, ...)
{
	va_list ap;

	fprintf(stderr, "  %-13s  - ", name);

	va_start(ap, descr);
	vfprintf(stderr, descr, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

static void usage_global_opts(void)
{
	fprintf(stderr, "\nGlobal options:\n");
	print_option("-h <hostname>",
		     "hostname to connect to (default: %s)", DEFAULT_HOSTNAME);
	print_option("-p <port>",
		     "TCP port to connect to (default: %hu)", DEFAULT_PORT);
}

static __attribute__ ((format (printf, 1, 2))) void usage(char *msg, ...)
{
	int i;

	if (msg) {
		va_list ap;

		fprintf(stderr, "Error: ");

		va_start(ap, msg);
		vfprintf(stderr, msg, ap);
		va_end(ap);

		fprintf(stderr, ".\n\n");
	}

	fprintf(stderr, "Usage: %s [<global opts>] <command>\n\n", prog);
	fprintf(stderr, "Available commands:\n");

	for (i = 0; i < ARRAY_LEN(cmdtbl); i++)
		fprintf(stderr, "\t%-13s - %s\n", cmdtbl[i].name,
			cmdtbl[i].summary);

	usage_global_opts();

	exit(1);
}

static void cmd_usage(const struct cmd *cmd)
{
	fprintf(stderr, "Usage: %s [<global opts>] %s\n", prog, cmd->name);

	usage_global_opts();

	exit(1);
}

int main(int argc, char **argv)
{
	bool got_host;
	bool got_port;
	int opt;
	int i;

	prog = argv[0];

	got_host = false;
	got_port = false;
	while ((opt = getopt(argc, argv, "+h:p:")) != -1) {
		switch (opt) {
			case 'h':
				if (got_host)
					usage("-h can be used only once");

				hostname = optarg;
				got_host = true;
				break;
			case 'p':
				if (got_port)
					usage("-p can be used only once");

				if (str2u16(optarg, &port)) {
					usage("'%s' is not a valid port number",
						optarg);
				}
				got_port = true;
				break;
			default:
				usage(NULL);
		}
	}

	/* missing command? */
	if (optind == argc)
		usage(NULL);

	for (i = 0; i < ARRAY_LEN(cmdtbl); i++) {
		if (!strcmp(argv[optind], cmdtbl[i].name)) {
			int ret;

			ret = cmdtbl[i].fxn(argc, argv);
			if (ret == PRINT_USAGE) {
				cmd_usage(&cmdtbl[i]);
				return -1;
			}

			ASSERT3S(ret, ==, 0);

			return 0;
		}
	}

	usage("unknown command '%s'", argv[optind]);
}
