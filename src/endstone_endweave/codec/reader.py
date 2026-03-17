"""Binary packet reader for Bedrock protocol deserialization."""

from __future__ import annotations

import struct


class PacketReader:
    """Reads binary data from a Bedrock packet payload."""

    __slots__ = ("_data", "_pos")

    def __init__(self, data: bytes) -> None:
        self._data = data
        self._pos = 0

    @property
    def position(self) -> int:
        return self._pos

    @position.setter
    def position(self, value: int) -> None:
        self._pos = value

    def has_remaining(self) -> bool:
        return self._pos < len(self._data)

    def remaining(self) -> int:
        return len(self._data) - self._pos

    def skip(self, n: int) -> None:
        self._pos += n

    def read_byte(self) -> int:
        val = self._data[self._pos]
        self._pos += 1
        return val

    def read_bytes(self, n: int) -> bytes:
        val = self._data[self._pos : self._pos + n]
        self._pos += n
        return val

    def read_remaining(self) -> bytes:
        val = self._data[self._pos :]
        self._pos = len(self._data)
        return val

    def read_bool(self) -> bool:
        return self.read_byte() != 0

    def read_short_le(self) -> int:
        val = struct.unpack_from("<h", self._data, self._pos)[0]
        self._pos += 2
        return val

    def read_ushort_le(self) -> int:
        val = struct.unpack_from("<H", self._data, self._pos)[0]
        self._pos += 2
        return val

    def read_int_le(self) -> int:
        val = struct.unpack_from("<i", self._data, self._pos)[0]
        self._pos += 4
        return val

    def read_int_be(self) -> int:
        val = struct.unpack_from(">i", self._data, self._pos)[0]
        self._pos += 4
        return val

    def read_uint_le(self) -> int:
        val = struct.unpack_from("<I", self._data, self._pos)[0]
        self._pos += 4
        return val

    def read_long_le(self) -> int:
        val = struct.unpack_from("<q", self._data, self._pos)[0]
        self._pos += 8
        return val

    def read_float_le(self) -> float:
        val = struct.unpack_from("<f", self._data, self._pos)[0]
        self._pos += 4
        return val

    def read_varint(self) -> int:
        """Read an unsigned variable-length integer (LEB128)."""
        result = 0
        shift = 0
        while True:
            b = self._data[self._pos]
            self._pos += 1
            result |= (b & 0x7F) << shift
            if (b & 0x80) == 0:
                break
            shift += 7
            if shift >= 35:
                raise ValueError("Varint too long")
        return result

    def read_signed_varint(self) -> int:
        """Read a signed variable-length integer (zigzag encoded)."""
        raw = self.read_varint()
        return (raw >> 1) ^ -(raw & 1)

    def read_varlong(self) -> int:
        """Read an unsigned variable-length long (LEB128, up to 64 bits)."""
        result = 0
        shift = 0
        while True:
            b = self._data[self._pos]
            self._pos += 1
            result |= (b & 0x7F) << shift
            if (b & 0x80) == 0:
                break
            shift += 7
            if shift >= 70:
                raise ValueError("Varlong too long")
        return result

    def slice_from(self, start: int) -> bytes:
        """Return bytes from start position to current position."""
        return self._data[start:self._pos]

    def read_string(self) -> str:
        """Read a varint-prefixed UTF-8 string."""
        length = self.read_varint()
        data = self._data[self._pos : self._pos + length]
        self._pos += length
        return data.decode("utf-8")
