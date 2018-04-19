/*
 * Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __NOMAD_OBJSTORE_POSIX_H
#define __NOMAD_OBJSTORE_POSIX_H

#include <nomad/types.h>

#include <nomad/objstore.h>
#include <nomad/objstore_backend.h>

/*
 * This is an objstore backend that uses a POSIX file system for storage of
 * objects.
 *
 * The layout is relatively simple.  Assuming /data is the path of the
 * vdev, we find:
 *
 * /data                   - the data dir
 * /data/vdev              - vdev info (uuid)
 * /data/<volid>           - volume
 * /data/<volid>/vol       - volume info (root OID, uuid, OID bmap, etc.)
 * /data/<volid>/<oid>     - everything related to the object
 * /data/<volid>/<oid>/<clock> - one version of the object
 *
 * Each /data/<volid>/<oid>/<clock> file contains a header (containing
 * attributes, etc.) followed by the object contents.
 */

struct posixvdev {
	struct objstore_vdev *vdev;

	int basefd;	/* base directory */

	/* FIXME: here to avoid compiler errors for now */
	int vdevfd;

	struct noid root;
};

extern const struct vol_ops posix_vol_ops;

extern int posix_new_obj(struct posixvdev *pv, uint16_t mode, struct noid *oid);

extern int oidbmap_create(struct posixvdev *pv);
extern int oidbmap_set(struct posixvdev *pv, uint64_t uniq);
extern int oidbmap_get_new(struct posixvdev *pv, uint64_t *new);
extern int oidbmap_put(struct posixvdev *pv, uint64_t uniq);

#endif
