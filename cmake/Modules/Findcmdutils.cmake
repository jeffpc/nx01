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

#
# Find the CMDUTILS includes and library.
#
# This module defines:
#   CMDUTILS_INCLUDE_DIR
#   CMDUTILS_LIBRARY
#   CMDUTILS_FOUND
#

find_path(CMDUTILS_INCLUDE_DIR sys/list.h)
find_library(CMDUTILS_LIBRARY NAMES cmdutils libcmdutils.so.1)

#
# Handle the QUIETLY and REQUIRED arguments and set CMDUTILS_FOUND to TRUE if
# all listed variables are TRUE.
#
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CMDUTILS DEFAULT_MSG CMDUTILS_LIBRARY
	CMDUTILS_INCLUDE_DIR)
