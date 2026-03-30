"""Binary packet reader for Bedrock protocol deserialization."""

import struct

_SHORT_LE = struct.Struct("<h")
_USHORT_LE = struct.Struct("<H")
_INT_LE = struct.Struct("<i")
_UINT_LE = struct.Struct("<I")
_INT_BE = struct.Struct(">i")
_INT64_LE = struct.Struct("<q")
_FLOAT_LE = struct.Struct("<f")
_DOUBLE_LE = struct.Struct("<d")


class PacketReader:
    """Reads binary data from a Bedrock packet payload.

    Attributes:
        _data: The raw packet bytes being read.
        _pos: Current read offset into _data.
    """

    def __init__(self, data: bytes) -> None:
        self._data = data
        self._pos = 0

    @property
    def position(self) -> int:
        return self._pos

    @position.setter
    def position(self, value: int) -> None:
        self._pos = value

    @property
    def has_remaining(self) -> bool:
        return self._pos < len(self._data)

    @property
    def remaining(self) -> int:
        return len(self._data) - self._pos

    def skip(self, n: int) -> None:
        """Advance the read position by n bytes without returning data.

        Args:
            n: Number of bytes to skip.

        Raises:
            ValueError: If n is negative or would move past end of data.
        """
        if n < 0 or n > self.remaining:
            raise ValueError(f"skip({n}) out of bounds (remaining={self.remaining})")
        self._pos += n

    def read_byte(self) -> int:
        """Read a single unsigned byte (uint8)."""
        val = self._data[self._pos]
        self._pos += 1
        return val

    def read_bytes(self, n: int) -> bytes:
        """Read exactly n raw bytes.

        Args:
            n: Number of bytes to read.

        Raises:
            ValueError: If n is negative or exceeds remaining bytes.
        """
        if n < 0 or n > self.remaining:
            raise ValueError(f"read_bytes({n}) out of bounds (remaining={self.remaining})")
        val = self._data[self._pos : self._pos + n]
        self._pos += n
        return val

    def read_remaining(self) -> bytes:
        """Read all bytes from current position to end of payload."""
        val = self._data[self._pos :]
        self._pos = len(self._data)
        return val

    def read_bool(self) -> bool:
        """Read a single byte as a boolean (nonzero = True)."""
        return self.read_byte() != 0

    def read_short_le(self) -> int:
        """Read a signed 16-bit little-endian integer."""
        val: int = _SHORT_LE.unpack_from(self._data, self._pos)[0]
        self._pos += 2
        return val

    def read_ushort_le(self) -> int:
        """Read an unsigned 16-bit little-endian integer."""
        val: int = _USHORT_LE.unpack_from(self._data, self._pos)[0]
        self._pos += 2
        return val

    def read_int_le(self) -> int:
        """Read a signed 32-bit little-endian integer."""
        val: int = _INT_LE.unpack_from(self._data, self._pos)[0]
        self._pos += 4
        return val

    def read_int_be(self) -> int:
        """Read a signed 32-bit big-endian integer."""
        val: int = _INT_BE.unpack_from(self._data, self._pos)[0]
        self._pos += 4
        return val

    def read_uint_le(self) -> int:
        """Read an unsigned 32-bit little-endian integer."""
        val: int = _UINT_LE.unpack_from(self._data, self._pos)[0]
        self._pos += 4
        return val

    def read_int64_le(self) -> int:
        """Read a signed 64-bit little-endian integer."""
        val: int = _INT64_LE.unpack_from(self._data, self._pos)[0]
        self._pos += 8
        return val

    def read_float_le(self) -> float:
        """Read a 32-bit little-endian IEEE 754 float."""
        val: float = _FLOAT_LE.unpack_from(self._data, self._pos)[0]
        self._pos += 4
        return val

    def read_double_le(self) -> float:
        """Read a 64-bit little-endian IEEE 754 float."""
        val: float = _DOUBLE_LE.unpack_from(self._data, self._pos)[0]
        self._pos += 8
        return val

    def read_uvarint(self) -> int:
        """Read an unsigned variable-length integer (LEB128)."""
        b = self._data[self._pos]
        self._pos += 1
        if b < 0x80:
            return b
        result = b & 0x7F
        shift = 7
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

    def read_varint(self) -> int:
        """Read a signed variable-length integer (zigzag encoded)."""
        raw = self.read_uvarint()
        return (raw >> 1) ^ -(raw & 1)

    def read_uvarint64(self) -> int:
        """Read an unsigned variable-length long (LEB128, up to 64 bits)."""
        b = self._data[self._pos]
        self._pos += 1
        if b < 0x80:
            return b
        result = b & 0x7F
        shift = 7
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

    def read_varint64(self) -> int:
        """Read a signed variable-length long (zigzag encoded, up to 64 bits)."""
        raw = self.read_uvarint64()
        return (raw >> 1) ^ -(raw & 1)

    _MAX_STRING_BYTES = 131068  # 32767 * 4

    def read_string(self) -> str:
        """Read a varint-prefixed UTF-8 string.

        Raises:
            ValueError: If the length prefix exceeds the max string byte limit.
        """
        length = self.read_uvarint()
        if length > self._MAX_STRING_BYTES:
            raise ValueError(f"String length {length} exceeds limit {self._MAX_STRING_BYTES}")
        return self.read_bytes(length).decode("utf-8")
