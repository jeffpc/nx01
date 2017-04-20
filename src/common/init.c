/*
 * Copyright (c) 2015-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

/*
 * Extract the "host-id" value from the config and start using it.
 */
static int __set_host_id(struct val *cfg)
{
	struct val *tmp;
	int ret;

	tmp = sexpr_alist_lookup_val(cfg, "host-id");
	if (!tmp) {
		cmn_err(CE_CRIT, "config is missing host-id");
		return -EINVAL;
	}

	if (tmp->type != VT_INT) {
		cmn_err(CE_CRIT, "config has non-integer host-id");
		ret = -EINVAL;
	} else if (!tmp->i) {
		cmn_err(CE_CRIT, "config has zero host-id");
		ret = -EINVAL;
	} else {
		ret = nomad_set_local_node_id(tmp->i);
	}

	val_putref(tmp);

	return ret;
}

static int load_config(void)
{
	struct val *cfg;
	char *fname;
	char *raw;
	int ret;

	fname = getenv(CFG_ENV_NAME);
	if (!fname)
		fname = DEFAULT_CFG_FILENAME;

	raw = read_file(fname);
	if (IS_ERR(raw))
		return PTR_ERR(raw);

	cfg = sexpr_parse(raw, strlen(raw));
	if (!cfg) {
		cmn_err(CE_CRIT, "failed to parse config (%s)", fname);
		ret = -EINVAL;
		goto err;
	}

	ret = __set_host_id(cfg);

err:
	/*
	 * free the whole alist & raw string
	 */
	val_putref(cfg);
	free(raw);

	return ret;
}

static void __attribute__((constructor)) common_init(void)
{
	int ret;

	ret = load_config();
	if (ret)
		cmn_err(CE_PANIC, "Failed to load nomad config: %s",
			xstrerror(ret));

	ret = nvclock_init_subsys();
	if (ret)
		cmn_err(CE_PANIC, "Failed to initialize nvclock subsystem: %s",
			xstrerror(ret));
}
