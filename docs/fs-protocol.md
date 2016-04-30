This document describes the RPC protocol between the client daemon and the
fs component.  It is still a work-in-progress.

Currently, we use `rpcgen(1)` created XDR encoding.

Each RPC request begins with a `struct rpc_header_req`.  It is then followed
by a variable number of bytes representing the rest of the request.  The
exact layout of this additional payload depends on the `opcode` in the
request header.

```C
struct rpc_header_req {
	uint16_t opcode;
};
```

Each RPC reply starts with a `struct rpc_header_res` which contains a status
code.  In the case of success (status code is 0), more data may follow
depending on the `opcode` in the request header.

```C
struct rpc_header_res {
	uint32_t err;
};
```

The following list of RPC commands does not explicitly list the request and
response headers since they are always present.  It only lists the
additional fields that follow.


NOP (0x0000)
============

This is a simple no-operation.

Inputs
------
None.

Output
------
None.

Limitations
-----------
None.


LOGIN (0x0001)
==============

The first command sent to the client daemon by the fs component.  It informs
the daemon of a mount request.  Eventually, this will also contain
credentials checking, etc. (hence the name).

Inputs
------
* conn name
* vg name

Outputs
-------
* oid of root

Limitations
-----------
At most one successful LOGIN is allowed per connection.  All LOGIN attempts
after a successful LOGIN will fail with `EALREADY`.


GETATTR (0x0002)
================

Get attributes (`struct nattr`) for of an object identified an open file
handle.

Inputs
------
* open file handle

Outputs
-------
* attributes

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


LOOKUP (0x0003)
===============

Given a directory open file handle and a path component (string), do a
lookup of the path component in the directory.

Inputs
------
* directory open file handle
* path component name

Outputs
-------
* child oid

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


CREATE (0x0004)
===============

Given a directory open file handle, a path component (string), and mode
(both type and access bits) create the new path component returning the oid
of the newly created file.  Creating an already existing path component
fails with `EEXIST`.

Inputs
------
* directory open file handle
* path component name
* mode (see `NATTR_*`)

Outputs
-------
* new file/dir/etc.'s oid

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


UNLINK (0x0005)
===============

Given a directory open file handle and a path component (string), remove the
path component from the directory.  Removing a non-existent path component
fails with `ENOENT`.

Inputs
------
* directory open file handle
* path component name

THOUGHT: Should this RPC also take an oid/vector clock of the object we want
to remove?

Outputs
-------
None.

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


OPEN (0x0006)
=============

Given a version of an object (oid and vector clock), open the specified
object and return an open file handle.

If a non-null vector clock is specified, only that version of the object is
considered.  If a null vector clock is specificied and there is only one
version of the object, the attributes of that version are returned.  If a
null vector clock is specified and there are multiple version of the object,
the operation fails with `ENOTUNIQ`.

Inputs
------
* oid
* vector clock

Outputs
-------
* open file handle

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


CLOSE (0x0007)
==============

Close an open file handle.  If the handle supplied has not be open via the
OPEN RPC, this operation fails with `EINVAL`.

Inputs
------
* open file handle

Outputs
-------
None.

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


READ (0x0008)
=============

Read a portion of an open object.  Trying to read from a directory fails with
`EISDIR`.  If the requested number of bytes would read beyond the end of the
object, only the data between the requested offset and the end of the object
is returned.  (Therefore, reading with an offset that is greater than or
equal to the object size will yield zero bytes.)

Inputs
------
* open file handle
* offset into the object version's data
* length (in bytes) to read

Outputs
-------
* length of data
* data

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


WRITE (0x0009)
==============

Write a portion of an open object.  Trying to write to a directory fails with
`EISDIR`.  Writing past the end of the object succeeds and the object length is
updated automatically.

Inputs
------
* open file handle
* offset into the object version's data
* length (in bytes) to write
* data

Outputs
-------
None.

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


SETATTR (0x000A)
================

Set selected attributes on an open object.  Even though the entire attribute
structure is sent, only the attributes marked as valid are set.

Setting the file size either truncates the object to the specified size or
extends it padding it with zero bytes.

Inputs
------
* open file handle
* new attributes (`struct nattr`)
* size is valid
* mode is valid

Outputs
-------
None.

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


GETDENT (0x000B)
================

Get the directory entry at a specific offset in a directory.  Additionally,
it returns the size of the current entry.  Adding this size to the current
offset will yield the offset of the next entry.

Inputs
------
* directory open file handle
* directory offset

Outputs
-------
* child oid
* child name
* entry size

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


Other RPCs that may end up useful
=================================

* RENAME - rename a "file"
* LINK - create a symlink or a hardlink
