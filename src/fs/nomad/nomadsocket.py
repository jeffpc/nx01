from collections import namedtuple
import datetime
import nomad.xdrsock

# TODO: use constantsgen
NOP = 0
LOGIN = 1
STAT = 2
LOOKUP = 3
CREATE = 4
REMOVE = 5

# TODO: move this somewhere else
attributes = namedtuple("attributes", [
    "mode",
    "nlink",  # TODO: what is this?
    "size",
    "access_time",
    "birth_time",
    "creation_time",
    "modification_time",
])

ZERO = datetime.timedelta(0)


class UTC(datetime.tzinfo):
    """UTC"""

    def utcoffset(self, dt):
        return ZERO

    def tzname(self, dt):
        return "UTC"

    def dst(self, dt):
        return ZERO


class NomadSocket(object):

    def __init__(self, sock):
        self.conn = nomad.xdrsock.XDRSock(sock)

    def nop(self):
        self._send_header(NOP)
        self._recv_header()

    def login(self, conn_name, vg_name):
        self._send_header(LOGIN)
        self.conn.send_string(conn_name)
        self.conn.send_string(vg_name)

        self._recv_header()
        return self._recv_handle()

    def stat(self, handle):
        self._send_header(STAT)
        self._send_handle(handle)

        self._recv_header()
        return attributes(
            mode=self.conn.recv_u32(),
            nlink=self.conn.recv_u32(),
            size=self.conn.recv_u64(),
            atime=self._recv_timestamp(),
            btime=self._recv_timestamp(),
            ctime=self._recv_timestamp(),
            mtime=self._recv_timestamp(),
        )

    def lookup(self, parent_handle, name):
        self._send_header(LOOKUP)
        self._send_handle(parent_handle)
        self.conn.send_string(name)

        self._recv_header()
        return self._recv_handle()

    def create(self, parent_handle, name, mode):
        self._send_header(CREATE)
        self._send_handle(parent_handle)
        self.conn.send_string(name)
        self.conn.send_u32(mode)

        self._recv_header()
        return self._recv_handle()

    def remove(self, parent_handle, name):
        self._send_header(REMOVE)
        self._send_handle(parent_handle)
        self.conn.send_string(name)

        # TODO: what return codes? only exceptions?
        self._recv_header()

    def _send_header(self, command):
        self.conn.send_u32(command)

    def _recv_header(self):
        value = self.conn.recv_u32()
        # TODO: raise on nonzero instead? errors map?
        assert value == 0
        return value

    # TODO: may want parsing of structures; can treat as opaque currently
    #def _send_object_id(self, object_id):
    #    self.conn.send_fixed_string(object_id)

    #def _recv_object_id(self):
    #    # See xdr_noid: u32 and u64
    #    return self.conn.recv_fixed_string(12)

    def _send_handle(self, handle):
        self.conn.send_fixed_string(handle)

    def _recv_handle(self):
        # oid - 12: u32 and u64
        # clock - 256: 16 * 2 * u64
        return self.conn.recv_fixed_string(12 + 256)

    def _recv_timestamp(self):
        # Timestamps are returned in nanoseconds.
        return datetime.datetime.fromtimestamp(self.conn.recv_u64() / 10 ** 9,
                                               UTC())

