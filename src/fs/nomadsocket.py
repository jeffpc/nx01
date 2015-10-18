import xdrsock

# TODO: use constantsgen
NOP = 0
LOGIN = 1


class NomadSocket(object):

    def __init__(self, sock):
        self.conn = xdrsock.XDRSock(sock)

    def _send_header(self, command):
        self.conn.send_u32(command)

    def nop(self):
        self._send_header(NOP)
        assert self.conn.recv_u32() == 0
