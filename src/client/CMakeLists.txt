#
# Copyright (c) 2015-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

add_executable(nomad-client
	main.c
	cmds.c
	ohandle.c

	# assorted RPC handlers
	cmd_dir.c
	cmd_login.c
	cmd_nop.c
	cmd_obj.c
	cmd_vdev.c
)

target_link_libraries(nomad-client
	${BASE_LIBS}
	${AVL_LIBRARY}
	common
	nomad_objstore
	nsl
)

install(TARGETS nomad-client DESTINATION sbin
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	            GROUP_READ GROUP_EXECUTE
		    WORLD_READ GROUP_EXECUTE)
