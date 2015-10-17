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

set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -Werror")

macro(xdr_check var op args)
	check_c_source_compiles("
#include <rpc/rpc.h>

static bool_t fxn(XDR *xdr, ${args})
{
	return FALSE;
}

static const struct xdr_ops ops = {
	.x_${op} = fxn,
};

int main()
{
	return 0;
}
" ${var})
endmacro()

xdr_check(HAVE_XDR_GETBYTES_UINT_ARG "getbytes" "caddr_t addr, u_int len")
xdr_check(HAVE_XDR_PUTBYTES_CONST_CHAR_ARG "putbytes" "const char *addr, u_int len")
xdr_check(HAVE_XDR_PUTINT32_CONST_ARG "putint32" "const int32_t *ptr")
xdr_check(HAVE_XDR_PUTLONG_CONST_ARG "putlong" "const long *ptr")
