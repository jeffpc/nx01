This document describes the RPC protocol between the client daemon and the
fs component.  It is still a work-in-progress.

Each RPC message is made up of 128-byte message followed by zero or more data
bytes.  The data bytes are consumed in the order that the fields in the fixed
size message are defined.  All multi-byte integers are stored in big endian.

The first two bytes of the message define an "opcode".

struct msg {
	uint16_t opcode;
	uint8_t opaque[126];
};

The interpretation of the opaque portion of the message varies based on the
opcode.

Each RPC returns a status code and possibly additional data.  The following
list of RPC commands does not explicitly list the status code since it is
always present.

Many of the RPC commands use an object handle (struct nobjhndl) which is
just a shorthand for a oid combined with a vector clock.


LOGIN (0x0001)
==============

The first command sent to the client daemon by the fs component.  It informs
the daemon of a mount request.  Eventually, this will also contain
credentials checking, etc. (hence the name).

	arg:
	 - conn name
	 - vg name

	ret:
	 - obj handle of root


STAT (0x0002)
=============

Get attributes (struct nattr) for a specific version of an object.

	arg:
	 - obj handle

	ret:
	 - attributes


LOOKUP (0x0003)
===============

Given a directory (handle) and a path component (string), do a lookup of the
path component in the directory.

	arg:
	 - directory/parent obj handle
	 - path component name

	ret:
	 - child obj handle

Is the return value flawed?  What if someone replaces a file?  E.g.,

$ touch foo
$ rm foo
$ touch foo

THOUGHT: The second "foo" will have a totally different oid.  Does this mean
that the directory has to keep track of the previous oids?  Maybe not
because the directory is an object as well and therefore it is versioned
too.  The using replacing a file (e.g., foo) with a new one will cause the
directory version to get incremented.  Since the directory didn't get
replaced (it got modified only), it's oid will stay the same.


CREATE (0x0004)
===============

Given a directory (obj handle), a path component (string), and mode (both
type and access bits) create the new path component returning the handle of
the newly created file.  Creating an already existing path component fails
with EEXIST.

	arg:
	 - directory/parent obj handle
	 - path component name
	 - mode (see NATTR_*)

	ret:
	 - new file/dir/etc.'s obj handle


REMOVE (0x0005)
===============

Given a directory (obj handle) and a path component (string), remove the
path component from the directory.  Removing a non-existent path component
fails with ENOENT.

	arg:
	 - directory/parent obj handle
	 - path component name

THOUGHT: Should this RPC also take an obj handle of the object we want to
remove?


Other RPCs that may end up useful
=================================

OPEN - open a file/dir
CLOSE - close a file/dir
READ - read an open file
WRITE - write an open file
