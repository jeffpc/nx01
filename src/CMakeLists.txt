#
# Copyright (c) 2015-2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

if(NOT UMEM_FOUND)
	set(UMEM_LIBRARY fakeumem)
	include_directories(fakeumem/include)
	add_subdirectory(fakeumem)
endif()

if(NOT AVL_FOUND)
	set(AVL_LIBRARY sunavl)
	include_directories(sunavl/include)
	add_subdirectory(sunavl)
endif()

set(LIST_LIBRARY sunlist)
set(TASKQ_LIBRARY suntaskq)

#
# From this point on, use these variables instead of hardcoding lib names:
#
#  AVL_LIBRARY
#  JEFFPC_LIBRARY
#  LIST_LIBRARY
#  TASKQ_LIBRARY
#  UMEM_LIBRARY
#

#
# We link all the binaries against these libs
#
set(BASE_LIBS
	${UMEM_LIBRARY}
	pthread
)

include_directories(
	common/include
	objstore/include
	sunlist/include
	suntaskq/include
	${JEFFPC_INCLUDE_DIR}
)

add_subdirectory(common)
add_subdirectory(sunlist)
add_subdirectory(suntaskq)
add_subdirectory(objstore)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(admin)
add_subdirectory(tool)
