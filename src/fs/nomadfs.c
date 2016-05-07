/*
 * Copyright (c) 2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#define FUSE_USE_VERSION 26

#include <fuse_lowlevel.h>

#include <jeffpc/error.h>
#include <jeffpc/hexdump.h>

#include <nomad/rpc.h>

#include "fscall.h"

#define REPLY_TIMEOUT	1.0
#define ATTR_TIMEOUT	1.0
#define ENTRY_TIMEOUT	1.0

/*
 * fuse assumes that the root inode number is FUSE_ROOT_ID.  Since that's a
 * valid oid uniq value, we swap the two inode numbers.
 */
static fuse_ino_t root_ino_buddy;
static struct fscall_state state;

static inline void make_oid(struct noid *oid, fuse_ino_t ino)
{
	if (ino == FUSE_ROOT_ID)
		ino = root_ino_buddy;
	else if (ino == root_ino_buddy)
		ino = FUSE_ROOT_ID;

	// XXX: use accessors
	noid_set(oid, state.root_oid.ds, ino);
}

static inline fuse_ino_t make_ino(const struct noid *oid)
{
	// XXX: use accessors
	if (oid->uniq == FUSE_ROOT_ID)
		return root_ino_buddy;
	if (oid->uniq == root_ino_buddy)
		return FUSE_ROOT_ID;
	return oid->uniq;
}

/*
 * Helpers to wrap fscall into more high-level operations
 */
static int __getattr(const struct noid *oid, struct nattr *nattr)
{
	uint32_t ohandle;
	int ret;

	ret = fscall_open(&state, oid, &ohandle);
	if (ret)
		return ret;

	ret = fscall_getattr(&state, ohandle, nattr);
	if (ret)
		goto err_close;

	return fscall_close(&state, ohandle);

err_close:
	fscall_close(&state, ohandle);

	return ret;
}

static int __create(fuse_req_t req, fuse_ino_t parent, const char *name,
		    uint16_t mode, struct noid *child_oid)
{
	uint32_t dir_ohandle;
	struct noid dir_oid;
	int ret;

	make_oid(&dir_oid, parent);

	ret = fscall_open(&state, &dir_oid, &dir_ohandle);
	if (ret)
		return ret;

	ret = fscall_create(&state, dir_ohandle, name, mode, child_oid);
	if (ret)
		goto err;

	return fscall_close(&state, dir_ohandle);

err:
	fscall_close(&state, dir_ohandle);

	return ret;
}

/*
 * Fuse operations
 */
static void nomadfs_getattr(fuse_req_t req, fuse_ino_t ino,
			    struct fuse_file_info *fi)
{
	struct nattr nattr;
	struct stat statbuf;
	struct noid oid;
	int ret;

	make_oid(&oid, ino);

	ret = __getattr(&oid, &nattr);
	if (ret)
		goto err;

	nattr_to_stat(&nattr, &statbuf);
	statbuf.st_ino = ino;

	/* XXX: somehow, we're returning good data, but gets interpreted wrong */
	/* XXX: seems to be a 32-bit. vs. 64-bit problem */
	fuse_reply_attr(req, &statbuf, REPLY_TIMEOUT);
	return;

err:
	fuse_reply_err(req, -nerr_to_errno(ret));
}

static void nomadfs_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	struct fuse_entry_param e;
	uint32_t child_ohandle;
	uint32_t dir_ohandle;
	struct noid child_oid;
	struct nattr nattr;
	int ret;

	if (parent == FUSE_ROOT_ID) {
		dir_ohandle = state.root_ohandle;
	} else {
		struct noid oid;

		make_oid(&oid, parent);

		ret = fscall_open(&state, &oid, &dir_ohandle);
		if (ret)
			goto err;
	}

	ret = fscall_lookup(&state, dir_ohandle, name, &child_oid);
	if (ret)
		goto err_close_dir;

	ret = fscall_open(&state, &child_oid, &child_ohandle);
	if (ret)
		goto err_close_dir;

	ret = fscall_getattr(&state, child_ohandle, &nattr);
	if (ret)
		goto err_close_child;

	ret = fscall_close(&state, child_ohandle);
	if (ret)
		goto err_close_dir;

	if (parent != FUSE_ROOT_ID)
		fscall_close(&state, dir_ohandle);

	memset(&e, 0, sizeof(e));
	nattr_to_stat(&nattr, &e.attr);
	e.ino = e.attr.st_ino = make_ino(&child_oid);
	e.attr_timeout = ATTR_TIMEOUT;
	e.entry_timeout = ENTRY_TIMEOUT;

	fuse_reply_entry(req, &e);

	return;

err_close_child:
	fscall_close(&state, child_ohandle);

err_close_dir:
	if (parent != FUSE_ROOT_ID)
		fscall_close(&state, dir_ohandle);

err:
	fuse_reply_err(req, -nerr_to_errno(ret));
}

static void nomadfs_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
			mode_t mode)
{
	struct fuse_entry_param e;
	struct noid child_oid;
	struct nattr nattr;
	int ret;

	ret = __create(req, parent, name, NATTR_DIR | (mode & 0777),
		       &child_oid);
	if (ret)
		goto err;

	ret = __getattr(&child_oid, &nattr);
	if (ret)
		goto err;

	memset(&e, 0, sizeof(e));
	nattr_to_stat(&nattr, &e.attr);
	e.ino = e.attr.st_ino = make_ino(&child_oid);
	e.attr_timeout = ATTR_TIMEOUT;
	e.entry_timeout = ENTRY_TIMEOUT;

	fuse_reply_entry(req, &e);

	return;

err:
	fuse_reply_err(req, -nerr_to_errno(ret));
}

static int reply_slice(fuse_req_t req, void *buf, size_t bufsize,
		       off_t fuse_off, size_t fuse_size)
{
	if (fuse_off < bufsize)
		return fuse_reply_buf(req, buf + fuse_off,
				      (bufsize - fuse_off < fuse_size) ?
				      (bufsize - fuse_off) : fuse_size);
	else
		return fuse_reply_buf(req, NULL, 0);
}

static int dirent_add(fuse_req_t req, void **_buf, size_t *_bufsize,
		      const char *name, fuse_ino_t ino)
{
	const size_t oldbufsize = *_bufsize;
	size_t bufsize = *_bufsize;
	struct stat statbuf;
	void *buf = *_buf;
	void *tmp;

	bufsize += fuse_add_direntry(req, NULL, 0, name, NULL, 0);

	tmp = realloc(buf, bufsize);
	if (!tmp)
		return -ENOMEM;
	*_buf = buf = tmp;
	*_bufsize = bufsize;

	memset(&statbuf, 0, sizeof(statbuf));
	statbuf.st_ino = ino;
	fuse_add_direntry(req, buf + oldbufsize, bufsize - oldbufsize, name,
			  &statbuf, bufsize);

	return 0;
}

static void nomadfs_readdir(fuse_req_t req, fuse_ino_t ino, size_t fuse_size,
			    off_t fuse_off, struct fuse_file_info *fi)
{
	uint32_t ohandle = fi->fh;
	uint64_t off;
	size_t bufsize;
	void *buf;
	int ret;

	bufsize = 0;
	buf = NULL;

	off = 0;

	ret = dirent_add(req, &buf, &bufsize, ".", FUSE_ROOT_ID);
	if (ret) {
		fuse_reply_err(req, -ret);
		return;
	}

	ret = dirent_add(req, &buf, &bufsize, "..", FUSE_ROOT_ID);
	if (ret) {
		fuse_reply_err(req, -ret);
		return;
	}

	do {
		uint64_t entry_size;
		struct noid oid;
		char *name;

		ret = fscall_getdent(&state, ohandle, off, &oid, &name, &entry_size);
		if (ret && ret != NERR_ENOENT)
			goto err;
		if (ret == NERR_ENOENT)
			break;

		ret = dirent_add(req, &buf, &bufsize, name, make_ino(&oid));
		if (ret) {
			fuse_reply_err(req, -ret);
			return;
		}

		off += entry_size;
	} while (!ret);

	reply_slice(req, buf, bufsize, fuse_off, fuse_size);

	free(buf);

	return;

err:
	fuse_reply_err(req, -nerr_to_errno(ret));
}

static void nomadfs_create(fuse_req_t req, fuse_ino_t parent, const char *name,
			   mode_t mode, struct fuse_file_info *fi)
{
	struct fuse_entry_param e;
	struct noid child_oid;
	struct nattr nattr;
	uint32_t ohandle;
	int ret;

	ret = __create(req, parent, name, NATTR_REG | (mode & 0777),
		       &child_oid);
	if (ret)
		goto err;

	ret = fscall_open(&state, &child_oid, &ohandle);
	if (ret)
		goto err;

	ret = fscall_getattr(&state, ohandle, &nattr);
	if (ret)
		goto err_close;

	fi->fh = ohandle;

	memset(&e, 0, sizeof(e));
	nattr_to_stat(&nattr, &e.attr);
	e.ino = e.attr.st_ino = make_ino(&child_oid);
	e.attr_timeout = ATTR_TIMEOUT;
	e.entry_timeout = ENTRY_TIMEOUT;

	fuse_reply_create(req, &e, fi);

	return;

err_close:
	fscall_close(&state, ohandle);

err:
	fuse_reply_err(req, -nerr_to_errno(ret));
}

static void nomadfs_open(fuse_req_t req, fuse_ino_t ino,
			 struct fuse_file_info *fi)
{
	uint32_t ohandle;
	struct noid oid;
	int ret;

	make_oid(&oid, ino);

	ret = fscall_open(&state, &oid, &ohandle);
	if (ret)
		goto err;

	fi->fh = ohandle;

	fuse_reply_open(req, fi);

	return;

err:
	fuse_reply_err(req, -nerr_to_errno(ret));
}

static void nomadfs_release(fuse_req_t req, fuse_ino_t ino,
			    struct fuse_file_info *fi)
{
	uint32_t ohandle = fi->fh;
	int ret;

	ret = fscall_close(&state, ohandle);
	if (ret)
		goto err;

	fuse_reply_err(req, 0);
	return;

err:
	fuse_reply_err(req, -nerr_to_errno(ret));
}

static void nomadfs_read(fuse_req_t req, fuse_ino_t ino, size_t size,
			 off_t off, struct fuse_file_info *fi)
{
	uint32_t ohandle = fi->fh;
	char *buf;
	int ret;

	buf = alloca(size);

	ret = fscall_read(&state, ohandle, buf, size, off);
	if (ret)
		goto err;

	{
		char tmp[16*2+1];
		hexdumpz(tmp, buf, 16, false);
		cmn_err(CE_DEBUG, "dump: %s", tmp);
	}

	fuse_reply_buf(req, buf, size);

	return;

err:
	fuse_reply_err(req, -nerr_to_errno(ret));
}

static void nomadfs_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
			  size_t size, off_t off, struct fuse_file_info *fi)
{
	uint32_t ohandle = fi->fh;
	int ret;

	ret = fscall_write(&state, ohandle, buf, size, off);
	if (ret)
		fuse_reply_err(req, -nerr_to_errno(ret));
	else
		fuse_reply_write(req, size);
}

static struct fuse_lowlevel_ops nomad_ops = {
	.getattr	= nomadfs_getattr,
	.lookup		= nomadfs_lookup,
	.mkdir		= nomadfs_mkdir,
	.readdir	= nomadfs_readdir,
	.create		= nomadfs_create,
	.open		= nomadfs_open,
	.release	= nomadfs_release,
	.opendir	= nomadfs_open,
	.releasedir	= nomadfs_release,
	.read		= nomadfs_read,
	.write		= nomadfs_write,
};

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_session *se;
	struct fuse_chan *ch;
	char *mountpoint;
	int ret;

	ret = fscall_connect("localhost", 2323, "myfiles", &state);
	if (ret)
		panic("failed to connect to nomad-client");

	/* see comment by root_ino_buddy definition */
	root_ino_buddy = state.root_oid.uniq; // XXX: use accessors

	ret = 1;

	if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) == -1)
		goto err;

	if ((ch = fuse_mount(mountpoint, &args)) == NULL)
		goto err;

	se = fuse_lowlevel_new(&args, &nomad_ops, sizeof(nomad_ops), NULL);
	if (!se)
		goto err_unmount;

	if (fuse_set_signal_handlers(se) == -1)
		goto err_session;

	fuse_session_add_chan(se, ch);
	ret = fuse_session_loop(se);
	fuse_remove_signal_handlers(se);
	fuse_session_remove_chan(ch);

err_session:
	fuse_session_destroy(se);

err_unmount:
	fuse_unmount(mountpoint, ch);

err:
	fuse_opt_free_args(&args);

	return ret ? 1 : 0;
}
