"""Binary packet writer for Bedrock protocol serialization."""

import struct

_SHORT_LE = struct.Struct("<h")
_USHORT_LE = struct.Struct("<H")
_INT_LE = struct.Struct("<i")
_UINT_LE = struct.Struct("<I")
_INT_BE = struct.Struct(">i")
_INT64_LE = struct.Struct("<q")
_FLOAT_LE = struct.Struct("<f")
_DOUBLE_LE = struct.Struct("<d")


class PacketWriter:
    """Builds binary data for a Bedrock packet payload.

    Attributes:
        _buf: The mutable byte buffer that accumulates written data.
    """

    def __init__(self) -> None:
        self._buf = bytearray()

    def to_bytes(self) -> bytes:
        """Return the accumulated buffer as an immutable bytes object."""
        return bytes(self._buf)

    def __len__(self) -> int:
        return len(self._buf)

    def write_byte(self, val: int) -> None:
        """Write a single unsigned byte (uint8)."""
        self._buf.append(val & 0xFF)

    def write_bytes(self, data: bytes) -> None:
        """Write raw bytes directly to the buffer."""
        self._buf.extend(data)

    def write_bool(self, val: bool) -> None:
        """Write a boolean as a single byte (1 or 0)."""
        self._buf.append(1 if val else 0)

    def write_short_le(self, val: int) -> None:
        """Write a signed 16-bit little-endian integer."""
        self._buf.extend(_SHORT_LE.pack(val))

    def write_ushort_le(self, val: int) -> None:
        """Write an unsigned 16-bit little-endian integer."""
        self._buf.extend(_USHORT_LE.pack(val))

    def write_int_le(self, val: int) -> None:
        """Write a signed 32-bit little-endian integer."""
        self._buf.extend(_INT_LE.pack(val))

    def write_int_be(self, val: int) -> None:
        """Write a signed 32-bit big-endian integer."""
        self._buf.extend(_INT_BE.pack(val))

    def write_uint_le(self, val: int) -> None:
        """Write an unsigned 32-bit little-endian integer."""
        self._buf.extend(_UINT_LE.pack(val))

    def write_int64_le(self, val: int) -> None:
        """Write a signed 64-bit little-endian integer."""
        self._buf.extend(_INT64_LE.pack(val))

    def write_float_le(self, val: float) -> None:
        """Write a 32-bit little-endian IEEE 754 float."""
        self._buf.extend(_FLOAT_LE.pack(val))

    def write_double_le(self, val: float) -> None:
        """Write a 64-bit little-endian IEEE 754 float."""
        self._buf.extend(_DOUBLE_LE.pack(val))

    def write_uvarint(self, val: int) -> None:
        """Write an unsigned variable-length integer (LEB128)."""
        val &= 0xFFFFFFFF
        if val < 0x80:
            self._buf.append(val)
            return
        while True:
            byte = val & 0x7F
            val >>= 7
            if val:
                self._buf.append(byte | 0x80)
            else:
                self._buf.append(byte)
                break

    def write_varint(self, val: int) -> None:
        """Write a signed variable-length integer (zigzag encoded)."""
        self.write_uvarint((val << 1) ^ (val >> 31))

    def write_varint64(self, val: int) -> None:
        """Write a signed variable-length long (zigzag encoded, up to 64 bits)."""
        self.write_uvarint64((val << 1) ^ (val >> 63))

    def write_uvarint64(self, val: int) -> None:
        """Write an unsigned variable-length long (LEB128, up to 64 bits)."""
        val &= 0xFFFFFFFFFFFFFFFF
        if val < 0x80:
            self._buf.append(val)
            return
        while True:
            byte = val & 0x7F
            val >>= 7
            if val:
                self._buf.append(byte | 0x80)
            else:
                self._buf.append(byte)
                break

    def write_string(self, val: str) -> None:
        """Write a varint-prefixed UTF-8 string."""
        encoded = val.encode("utf-8")
        self.write_uvarint(len(encoded))
        self._buf.extend(encoded)
