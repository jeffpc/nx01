This document describes the overall architecture of Nomad.


Object Store Concepts
=====================

Both clients and servers use the same object storage library and backend
modules.  The library implements a generic object-based storage engine which
provides a straightforward API that handles most of the storage logic
necessary.  The actual details of how to store and retrieve objects from
storage are delegated to backend modules.  This provides a lot of
flexibility when it comes to deployment and system administration.

There are a couple of concepts one must understand first.

Object IDs
----------

An object ID is a 128-bit unique identifier.  It should be treated as an
opaque value.  (`struct noid`)

Vector Clocks
-------------

A vector clock is a mechanism used for versioning of of data.  It consists
of an array of node and sequence id pairs.  (`struct nvclock`)

Object Versions
---------------

An object version is a buffer consisting of user data and a set of
attributes (several timestamps, permissions, size, etc.).  This data can be
either a raw byte sequence, or it can be backend-private format used to
represent other file types (e.g., directories).  Each version of an object
is identified by a vector clock.  When the user data or attributes are
modified, the corresponding vector clock is updated to reflect the presence
of a change.

Objects
-------

An object is a collection of object versions.  The object itself is
identified by an object ID.  Most of the time, objects have only one
version.  Multiple versions come into being due to conflicting updates
during disconnected operation.

Open File Handles
-----------------

In order to modify an object version's data, we must "open" it.  This
opening places a hold on the version, which allows for POSIX-style file I/O
even when the file has been unlinked from the name space.  The hold is
referred to by clients via a 32-bit unsigned non-zero integer called the
open file handle.  (`uint32_t`)  Closing the open file handle releases the
hold.  Technically, open file handles are not part of the object store
library.  Instead, the library return an opaque cookie (`void *`) when an
object is open.  It is up to the consumer to remember this cookie and use it
on subsequent operations.  At the very least, the client daemon maps the
cookies to randomly generated open file handles that it presents to its RPC
layer users.

Volumes
-------

A volume is device which can store objects.  The exact details how the
objects are store is abstracted away by the backend modules API.  For
example, a backend can use a raw disk device or directory on a
POSIX-compliant file system.

Volume Groups
-------------

A volume group is a set of volumes grouped by the administrator to serve a
single purpose.  More than one volume group can be present on a system.  For
example, two teams can use the same system but each can use its own volume
group to separate disk I/O and capacity.

Note: For prototyping reasons, a volume group is limited to only one volume.
Therefore, a volume group will have either zero or one volume.


Components
==========

Nomad is a distributed system consisting of a server and a number of
clients.  Clients and servers run similar but slightly different set of
software.

A typical client installation includes `nomad`, `nomadadm`, `nomad-client`,
and `fs`.

A typical server installation includes `nomadadm` and `nomad-server`.

`fs`
----

A FUSE module to expose the storage managed by `nomad-client` as a POSIX
file system.  It uses the protocol defined by `rpc_fs.x`.

`nomad-client`
--------------

A daemon that runs on client systems.  It manages server connections, and
local caching.  It also accepts RPCs from `fs` running on the same host and
fulfills them with data stored locally in the cache as well as remotely.

`nomad-server`
--------------

A daemon that runs on server systems.  It manages the volumes and volume
groups exposed to `nomad-client` clients.

Note: the protocol is currently not defined.

`nomadadm`
----------

A utility used to manage `nomad-client` and `nomad-server` on the local
system.

Note: the protocol is currently not defined.

`nomad`
-------

A utility used to query files on an `fs` mounted volume group.  It allows
the user to inspect some of the state that is not visible through POSIX
APIs.  It uses `ioctls` to get more information from `fs`.

`libnomad_objstore.so`
----------------------

This shared library implements a generic object storage layer.  It relies of
backend modules (`libnomad_objstore_*.so`) implement a specific storage
method.

It is in the form of a library so that both `nomad-client` and
`nomad-server` can use the same exact code.

Note: Eventually, the backend will be selectable at volume instantiation
time.  At the moment, this is not implemented and `nomad-client` hardcodes
the use of the mem backend (`libnomad_objstore_mem.so`).
