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

Many of the RPC commands use an object handle (`struct nobjhndl`) which is
just a shorthand for a oid combined with a vector clock.


NOP (0x0000)
============

This is a simple no-operation.

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


STAT (0x0002)
=============

Get attributes (`struct nattr`) for of an object.

If the handle specifies a non-null vector clock, only that version of the
object is considered.  If a null vector clock is specificied and there is
only one version of the object, the attributes of that version are returned.
If a null vector clock is specified and there are multiple version of the
object, the operation fails with `ENOTUNIQ`.

Inputs
------
* obj handle

Outputs
-------
* attributes

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


LOOKUP (0x0003)
===============

Given a directory (handle) and a path component (string), do a lookup of the
path component in the directory.

Inputs
------
* directory/parent obj handle
* path component name

Outputs
-------
* child oid

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


CREATE (0x0004)
===============

Given a directory (obj handle), a path component (string), and mode (both
type and access bits) create the new path component returning the handle of
the newly created file.  Creating an already existing path component fails
with `EEXIST`.

Inputs
------
* directory/parent obj handle
* path component name
* mode (see `NATTR_*`)

Outputs
-------
* new file/dir/etc.'s oid

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


REMOVE (0x0005)
===============

Given a directory (obj handle) and a path component (string), remove the
path component from the directory.  Removing a non-existent path component
fails with `ENOENT`.

Inputs
------
* directory/parent obj handle
* path component name

THOUGHT: Should this RPC also take an obj handle of the object we want to
remove?

Limitations
-----------
Fails with `EPROTO` if the client hasn't gotten a successful LOGIN.


Other RPCs that may end up useful
=================================

* OPEN - open a file/dir
* CLOSE - close a file/dir
* READ - read an open file
* WRITE - write an open file
