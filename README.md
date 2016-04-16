Nomad is a distributed file system supporting disconnected operation.

The current goals include:
* architecture and OS agnostic code
* client-server architecture
* untrusted client
* disconnected operation
* support for special files (named pipes, symlinks, etc.)
* Unix-style file permissions
* multi-user capable server

Once these are done, we will reevaluate the next set of goals.


Building and Installing
=======================

```sh
$ cmake -DCMAKE_INSTALL_PREFIX=/prefix .
$ make
$ make install
```

This will build and install the binaries and libraries under the specified
prefix.


Running
=======

First you need to create a config file.  For an example along with a
description of the syntax refer to `examples/nomad.conf`.

Either place the config file at /etc/nomad.conf or set the `NOMAD_CONFIG`
environment variable to the config file's location.  All nomad executables
that require a config file check the environment variable, and if not set
they fall back to the hardcoded `/etc/nomad.conf`.

For example:

```sh
$ export NOMAD_CONFIG=/some/where/nomad.conf
$ nomadadm host-id
0xabcdefabcdefabcd
```


Internal Dependencies
=====================

```text
             | sunavl | sunlist | fakeumem | suntaskq | common | objstore 
-------------+--------+---------+----------+----------+--------+----------
sunavl       |   -    |    n    |    n     |     n    |   n    |    n     
sunlist      |   n    |    -    |    n     |     n    |   n    |    n     
fakeumem     |   n    |    n    |    -     |     n    |   n    |    n     
suntaskq     |   n    |    n    |    y     |     -    |   n    |    n     
common       |   n    |    n    |    y     |     y    |   -    |    n     
objstore     |   n    |    y    |    y     |     n    |   n    |    -     
objs. module |   ?    |    ?    |    ?     |     ?    |   ?    |    n     
client       |   y    |    n    |    y     |     n    |   y    |    y     
server       |   n    |    n    |    y     |     n    |   n    |    n     
tool         |   n    |    n    |    y     |     n    |   n    |    n     

  y = yes, linked against
  n = no, not linked against
  - = not applicable
  ? = may be linked against as necessary
```

The above table assumes the lack of avl, cmdutils, and umem libraries on the
system.  If they are present, they are used instead of sunavl, sunlist, and
fakeumem respectively.
