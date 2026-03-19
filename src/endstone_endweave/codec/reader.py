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

    def read_uvarint(self) -> int:
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

    def read_varint(self) -> int:
        """Read a signed variable-length integer (zigzag encoded)."""
        raw = self.read_uvarint()
        return (raw >> 1) ^ -(raw & 1)

    def read_uvarint64(self) -> int:
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

    def read_varint64(self) -> int:
        """Read a signed variable-length long (zigzag encoded, up to 64 bits)."""
        raw = self.read_uvarint64()
        return (raw >> 1) ^ -(raw & 1)

    def skip_string(self) -> None:
        """Skip a varint-prefixed UTF-8 string without decoding."""
        length = self.read_uvarint()
        self._pos += length

    def skip_nbt_compound(self) -> None:
        """Skip a Bedrock network NBT compound tag (iterative).

        Advances the reader past the root tag without allocating data
        structures. Bedrock network NBT uses uvarint for string lengths
        and zigzag varint/varint64 for Int/Int64 values.

        Stack entries use (container_type, extra):
          compound (10): extra = -1 (read tag headers until End byte)
          list (9):      extra encodes elem_type and remaining count
                         as (elem_type << 32 | remaining)
        """
        _MAX_DEPTH = 512

        root_type = self.read_byte()
        if root_type == 0:
            return  # null/end tag -- empty
        if root_type != 10:
            raise ValueError(f"Expected compound root tag (10), got {root_type}")
        # Skip root tag name
        self.skip_string()

        # Stack: (container_type, packed_info)
        stack: list[tuple[int, int]] = [(10, -1)]

        while stack:
            if len(stack) > _MAX_DEPTH:
                raise ValueError("NBT nesting depth exceeded")

            container_type, info = stack[-1]

            if container_type == 10:
                # Compound: read next child tag header
                tag_type = self.read_byte()
                if tag_type == 0:  # End tag
                    stack.pop()
                    continue
                self.skip_string()  # tag name
            else:
                # List: consume next element
                elem_type = info >> 32
                remaining = info & 0xFFFFFFFF
                if remaining == 0:
                    stack.pop()
                    continue
                stack[-1] = (9, (elem_type << 32) | (remaining - 1))
                tag_type = elem_type

            # Skip value based on tag_type
            if tag_type == 1:      # Byte
                self._pos += 1
            elif tag_type == 2:    # Short
                self._pos += 2
            elif tag_type == 3:    # Int (zigzag varint)
                self.read_varint()
            elif tag_type == 4:    # Int64 (zigzag varint64)
                self.read_varint64()
            elif tag_type == 5:    # Float
                self._pos += 4
            elif tag_type == 6:    # Double
                self._pos += 8
            elif tag_type == 7:    # ByteArray (varint length + bytes)
                length = self.read_varint()
                self._pos += length
            elif tag_type == 8:    # String
                self.skip_string()
            elif tag_type == 9:    # List
                elem = self.read_byte()
                count = self.read_varint()
                if count > 0 and elem != 0:
                    stack.append((9, (elem << 32) | count))
            elif tag_type == 10:   # Compound
                stack.append((10, -1))
            elif tag_type == 11:   # IntArray (varint count + varints)
                count = self.read_varint()
                for _ in range(count):
                    self.read_varint()
            else:
                raise ValueError(f"Unknown NBT tag type: {tag_type}")

    def slice_from(self, start: int) -> bytes:
        """Return bytes from start position to current position."""
        return self._data[start:self._pos]

    def read_string(self) -> str:
        """Read a varint-prefixed UTF-8 string."""
        length = self.read_uvarint()
        data = self._data[self._pos : self._pos + length]
        self._pos += length
        return data.decode("utf-8")
