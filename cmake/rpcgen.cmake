#
# Copyright (c) 2015 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

macro(rpcgen src)
	set(xfile "${CMAKE_CURRENT_SOURCE_DIR}/${src}.x")
	set(cfile "${src}_xdr.c")
	set(hfile "include/nomad/${src}_xdr.h")

	# xdrgen(1) doesn't produce the cleanest code imaginable
	set_source_files_properties("${cfile}" PROPERTIES
		COMPILE_FLAGS "-Wno-unused-variable")

	add_custom_command(
		OUTPUT "${cfile}" "${hfile}"
		COMMAND rm -f "${cfile}" "${hfile}"
		COMMAND rpcgen -C -h -o "${hfile}" "${xfile}"
		COMMAND rpcgen -C -c -o "${cfile}" "${xfile}"
		DEPENDS "${xfile}"
	)
endmacro()
