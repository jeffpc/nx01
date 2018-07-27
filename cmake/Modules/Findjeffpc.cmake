#
# Copyright (c) 2016-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#
# Find the libjeffpc includes and library.
#
# This module defines:
#   JEFFPC_INCLUDE_DIR
#   JEFFPC_LIBRARY
#   JEFFPC_FOUND
#
# Use the following variables to help locate the library and header files.
#   WITH_JEFFPC_LIB=x      - directory containing libjeffpc.so
#   WITH_JEFFPC_INCLUDES=x - directory containing jeffpc/jeffpc.h
#   WITH_JEFFPC=x          - same as setting WITH_JEFFPC_LIB=x/lib and
#                            WITH_JEFFPC_INCLUDES=x/include
#

if (WITH_JEFFPC)
	if (NOT WITH_JEFFPC_LIB)
		set(WITH_JEFFPC_LIB "${WITH_JEFFPC}/lib")
	endif()
	if (NOT WITH_JEFFPC_INCLUDES)
		set(WITH_JEFFPC_INCLUDES "${WITH_JEFFPC}/include")
	endif()
endif()

find_path(JEFFPC_INCLUDE_DIR jeffpc/jeffpc.h
	PATHS ${WITH_JEFFPC_INCLUDES})
find_library(JEFFPC_LIBRARY NAMES jeffpc
	PATHS ${WITH_JEFFPC_LIB}
)

#
# Handle the QUIETLY and REQUIRED arguments and set JEFFPC_FOUND to TRUE if
# all listed variables are TRUE.
#
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JEFFPC DEFAULT_MSG JEFFPC_LIBRARY
	JEFFPC_INCLUDE_DIR)
