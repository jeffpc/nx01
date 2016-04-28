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

include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckCSourceCompiles)

include(cmake/config/xdr.cmake)

check_include_files(door.h HAVE_DOORS)

set(CMAKE_MODULE_PATH "${CMAKE_DIR}/Modules")
find_package(umem)
find_package(avl)
find_package(cmdutils)
find_package(jeffpc)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/common/include/nomad/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/src/common/include/nomad/config.h")

include_directories("${CMAKE_CURRENT_BINARY_DIR}/src/common/include")
