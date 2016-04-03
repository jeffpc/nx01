#
# Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
# Copyright (c) 2015 Joshua Kahn <josh@joshuak.net>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

rpcgen(rpc_fs)

add_library(nomad_common SHARED
	attr.c
	connsvc.c
	error.c
	init.c
	mutex.c
	node.c
	oid.c
	vclock.c
	xdrfd.c

	rpc_fs_xdr.c
)

target_link_libraries(nomad_common
	${BASE_LIBS}
	jeffpc
)

install(TARGETS nomad_common DESTINATION lib
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
install(FILES	include/nomad/attr.h
		include/nomad/init.h
		include/nomad/iter.h
		include/nomad/malloc.h
		include/nomad/mutex.h
		include/nomad/time.h
		include/nomad/types.h
		include/nomad/vclock.h
		${CMAKE_CURRENT_BINARY_DIR}/include/nomad/config.h
	DESTINATION include/nomad
	PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

# for convenience
add_library(common ALIAS nomad_common)
