/*
 * Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <jeffpc/jeffpc.h>
#include <jeffpc/error.h>
#include <jeffpc/io.h>
#include <jeffpc/sexpr.h>

#include <nomad/init.h>
#include <nomad/types.h>

#define DEFAULT_CFG_FILENAME	"/etc/nomad.conf"
#define CFG_ENV_NAME		"NOMAD_CONFIG"

static int load_config(void)
{
	struct val *cfg;
	struct val *tmp;
	char *fname;
	char *raw;

	fname = getenv(CFG_ENV_NAME);
	if (!fname)
		fname = DEFAULT_CFG_FILENAME;

	raw = read_file(fname);
	if (IS_ERR(raw))
		return PTR_ERR(raw);

	cfg = sexpr_parse(raw, strlen(raw));
	if (!cfg)
		panic("failed to parse config (%s)", fname);

	/*
	 * set the host-id
	 */
	tmp = sexpr_alist_lookup_val(cfg, "host-id");
	if (!tmp)
		panic("config (%s) is missing host-id", fname);

	if (tmp->type != VT_INT)
		panic("config (%s) has non-integer host-id", fname);

	VERIFY0(nomad_set_local_node_id(tmp->i));

	val_putref(tmp);

	/*
	 * free the whole alist & raw string
	 */
	val_putref(cfg);
	free(raw);

	return 0;
}

int common_init(void)
{
	int ret;

	jeffpc_init(NULL);

	ret = load_config();
	if (ret)
		return ret;

	return nvclock_init_subsys();
}
