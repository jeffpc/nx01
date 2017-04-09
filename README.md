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

First, you need to get the code itself.  Since Nomad is under heavy
development, there are no releases yet.  So, for now you have to clone it
using git:

```sh
$ git clone https://github.com/jeffpc/nx01.git
```

After you have cloned the repository, you will need to check out the
submodules:

```sh
$ git submodule init
$ git submodule update
```

Now that you have all the necessary code, you can build it.

```sh
$ cmake -DCMAKE_INSTALL_PREFIX=/prefix .
$ make
$ make install
```

This will build and install the binaries and libraries under the specified
prefix.

You can also define the following cmake variables to help it find libjeffpc.

	WITH_JEFFPC_LIB=<directory containing libjeffpc.so>
	WITH_JEFFPC_INCLUDES=<directory containing jeffpc/jeffpc.h>

Setting `WITH_JEFFPC` to `/path` yields the same effect as setting both:

	WITH_JEFFPC_LIB=/path/lib
	WITH_JEFFPC_INCLUDES=/path/include

For example:

```sh
$ cmake -DCMAKE_INSTALL_PREFIX=/prefix -DWITH_JEFFPC=/opt/jeffpc .
```


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
             | sunavl | sunlist | fakeumem | common | objstore 
-------------+--------+---------+----------+--------+----------
sunavl       |   -    |    n    |    n     |   n    |    n     
sunlist      |   n    |    -    |    n     |   n    |    n     
fakeumem     |   n    |    n    |    -     |   n    |    n     
common       |   n    |    n    |    y     |   -    |    n     
objstore     |   n    |    y    |    y     |   n    |    -     
objs. module |   ?    |    ?    |    ?     |   ?    |    n     
client       |   y    |    n    |    y     |   y    |    y     
server       |   n    |    n    |    y     |   n    |    n     
tool         |   n    |    n    |    y     |   n    |    n     

  y = yes, linked against
  n = no, not linked against
  - = not applicable
  ? = may be linked against as necessary
```

The above table assumes the lack of avl and umem libraries on the system.
If they are present, they are used instead of sunavl and fakeumem
respectively.
