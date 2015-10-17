# Copyright (c) 2015 Steve Dougherty <steve@asksteved.com>
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

import xdrlib


class XDRSock(object):

    def __init__(self, sock):
        """
        xdrlib socket wrapper.

        :type sock: socket.socket
        """
        self.sock = sock
        self.packer = xdrlib.Packer()

    def send_u32(self, value):
        self.packer.reset()
        self.packer.pack_uint(value)
        self.sock.sendall(self.packer.get_buffer())

    def send_u64(self, value):
        self.packer.reset()
        self.packer.pack_uhyper(value)
        self.sock.sendall(self.packer.get_buffer())

    def send_fixed_string(self, value):
        self.packer.reset()
        self.packer.pack_fstring(len(value), value)
        self.sock.sendall(self.packer.get_buffer())

    def send_string(self, value):
        self.packer.reset()
        self.packer.pack_string(value)
        self.sock.sendall(self.packer.get_buffer())

    def recv_u32(self):
        unpacker = xdrlib.Unpacker(self.__recv(4))
        return unpacker.unpack_uint()

    def recv_u64(self):
        unpacker = xdrlib.Unpacker(self.__recv(8))
        return unpacker.unpack_uhyper()

    def recv_fixed_string(self, length):
        if length % 4:
            padded_length = length + (4 - (length % 4))
        else:
            padded_length = length

        unpacker = xdrlib.Unpacker(self.__recv(padded_length))
        return unpacker.unpack_fstring(length)

    def recv_string(self):
        length = self.recv_u32()
        return self.recv_fixed_string(length)

    def __recv(self, num_bytes):
        chunks = []
        remaining_bytes = num_bytes
        while remaining_bytes > 0:
            chunk = self.sock.recv(remaining_bytes)
            remaining_bytes -= len(chunk)
            chunks.append(chunk)

        return b"".join(chunks)
