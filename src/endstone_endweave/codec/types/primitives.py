"""Primitive packet field types for read/write/passthrough operations.

Each Type knows how to read from a PacketReader and write to a PacketWriter,
enabling the PacketWrapper's passthrough() pattern (read + write in one call).

See Also:
    com.viaversion.viaversion.api.type.Type
"""

from abc import ABC, abstractmethod
from typing import Generic, TypeVar

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter

_T = TypeVar("_T")


class Type(ABC, Generic[_T]):
    """A serializable packet field type."""

    @abstractmethod
    def read(self, reader: PacketReader) -> _T:
        """Deserialize a value from the reader.

        Args:
            reader: The packet reader to read from.

        Returns:
            The deserialized value.
        """
        ...

    @abstractmethod
    def write(self, writer: PacketWriter, value: _T) -> None:
        """Serialize a value into the writer.

        Args:
            writer: The packet writer to write to.
            value: The value to serialize.
        """
        ...


class _Byte(Type[int]):
    """Unsigned byte (uint8)."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_byte()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_byte(value)


class _Bool(Type[bool]):
    """Boolean (single byte, nonzero = True)."""

    def read(self, reader: PacketReader) -> bool:
        return reader.read_bool()

    def write(self, writer: PacketWriter, value: bool) -> None:
        writer.write_bool(value)


class _ShortLE(Type[int]):
    """Signed 16-bit little-endian integer."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_short_le()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_short_le(value)


class _UShortLE(Type[int]):
    """Unsigned 16-bit little-endian integer."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_ushort_le()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_ushort_le(value)


class _IntLE(Type[int]):
    """Signed 32-bit little-endian integer."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_int_le()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_int_le(value)


class _IntBE(Type[int]):
    """Signed 32-bit big-endian integer."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_int_be()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_int_be(value)


class _UIntLE(Type[int]):
    """Unsigned 32-bit little-endian integer."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_uint_le()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_uint_le(value)


class _Int64LE(Type[int]):
    """Signed 64-bit little-endian integer."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_int64_le()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_int64_le(value)


class _FloatLE(Type[float]):
    """32-bit little-endian IEEE 754 float."""

    def read(self, reader: PacketReader) -> float:
        return reader.read_float_le()

    def write(self, writer: PacketWriter, value: float) -> None:
        writer.write_float_le(value)


class _DoubleLE(Type[float]):
    """64-bit little-endian IEEE 754 float."""

    def read(self, reader: PacketReader) -> float:
        return reader.read_double_le()

    def write(self, writer: PacketWriter, value: float) -> None:
        writer.write_double_le(value)


class _VarInt(Type[int]):
    """Signed variable-length integer (zigzag encoded, up to 32 bits)."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_varint()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_varint(value)


class _UVarInt(Type[int]):
    """Unsigned variable-length integer (LEB128, up to 32 bits)."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_uvarint()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_uvarint(value)


class _VarInt64(Type[int]):
    """Signed variable-length integer (zigzag encoded, up to 64 bits)."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_varint64()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_varint64(value)


class _UVarInt64(Type[int]):
    """Unsigned variable-length integer (LEB128, up to 64 bits)."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_uvarint64()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_uvarint64(value)


class _String(Type[str]):
    """Varint-prefixed UTF-8 string."""

    def read(self, reader: PacketReader) -> str:
        return reader.read_string()

    def write(self, writer: PacketWriter, value: str) -> None:
        writer.write_string(value)


class _Bytes(Type[bytes]):
    """Fixed-length raw bytes."""

    def __init__(self, length: int) -> None:
        """Initialize a fixed-length bytes type.

        Args:
            length: Exact number of bytes to read or expect.
        """
        self._length = length

    def read(self, reader: PacketReader) -> bytes:
        return reader.read_bytes(self._length)

    def write(self, writer: PacketWriter, value: bytes) -> None:
        writer.write_bytes(value)


class _RemainingBytes(Type[bytes]):
    """All remaining bytes in the packet."""

    def read(self, reader: PacketReader) -> bytes:
        return reader.read_remaining()

    def write(self, writer: PacketWriter, value: bytes) -> None:
        writer.write_bytes(value)


# Singleton type instances -- use these in handlers
BYTE = _Byte()
BOOL = _Bool()
SHORT_LE = _ShortLE()
USHORT_LE = _UShortLE()
INT_LE = _IntLE()
INT_BE = _IntBE()
UINT_LE = _UIntLE()
INT64_LE = _Int64LE()
FLOAT_LE = _FloatLE()
DOUBLE_LE = _DoubleLE()
VAR_INT = _VarInt()
UVAR_INT = _UVarInt()
VAR_INT64 = _VarInt64()
UVAR_INT64 = _UVarInt64()
STRING = _String()
REMAINING_BYTES = _RemainingBytes()


class _NetworkBlockPos(Type[tuple[int, int, int]]):
    """v924 NetworkBlockPosition: varint X, uvarint Y, varint Z."""

    def read(self, reader: PacketReader) -> tuple[int, int, int]:
        x = reader.read_varint()
        y = reader.read_uvarint()
        if y >= 0x80000000:
            y -= 0x100000000
        z = reader.read_varint()
        return (x, y, z)

    def write(self, writer: PacketWriter, value: tuple[int, int, int]) -> None:
        writer.write_varint(value[0])
        writer.write_uvarint(value[1])
        writer.write_varint(value[2])


class _BlockPos(Type[tuple[int, int, int]]):
    """v944 BlockPos: varint X, varint Y, varint Z."""

    def read(self, reader: PacketReader) -> tuple[int, int, int]:
        x = reader.read_varint()
        y = reader.read_varint()
        z = reader.read_varint()
        return (x, y, z)

    def write(self, writer: PacketWriter, value: tuple[int, int, int]) -> None:
        writer.write_varint(value[0])
        writer.write_varint(value[1])
        writer.write_varint(value[2])


class _Vec3(Type[tuple[float, float, float]]):
    """Vec3: three little-endian floats (X, Y, Z)."""

    def read(self, reader: PacketReader) -> tuple[float, float, float]:
        x = reader.read_float_le()
        y = reader.read_float_le()
        z = reader.read_float_le()
        return (x, y, z)

    def write(self, writer: PacketWriter, value: tuple[float, float, float]) -> None:
        writer.write_float_le(value[0])
        writer.write_float_le(value[1])
        writer.write_float_le(value[2])


class _Vec2(Type[tuple[float, float]]):
    """Vec2: two little-endian floats (X, Y)."""

    def read(self, reader: PacketReader) -> tuple[float, float]:
        x = reader.read_float_le()
        y = reader.read_float_le()
        return (x, y)

    def write(self, writer: PacketWriter, value: tuple[float, float]) -> None:
        writer.write_float_le(value[0])
        writer.write_float_le(value[1])


NETWORK_BLOCK_POS = _NetworkBlockPos()
BLOCK_POS = _BlockPos()
VEC3 = _Vec3()
VEC2 = _Vec2()
UUID = _Bytes(16)


class ArrayType(Type[list[_T]]):
    """Count-prefixed array wrapper for any Type.

    Reads a uvarint count, then N elements of the inner type.
    Writes a uvarint count followed by each element.

    See Also:
        com.viaversion.viaversion.api.type.types.ArrayType
    """

    def __init__(self, inner: Type[_T]) -> None:
        self._inner = inner

    def read(self, reader: PacketReader) -> list[_T]:
        count = reader.read_uvarint()
        return [self._inner.read(reader) for _ in range(count)]

    def write(self, writer: PacketWriter, value: list[_T]) -> None:
        writer.write_uvarint(len(value))
        for item in value:
            self._inner.write(writer, item)


class OptionalType(Type[_T | None]):
    """Bool-prefixed optional wrapper for any Type.

    Reads a boolean; if true, reads the inner type. If false, returns None.
    Writes a boolean prefix followed by the inner value (if not None).

    See Also:
        com.viaversion.viaversion.api.type.OptionalType
    """

    def __init__(self, inner: Type[_T]) -> None:
        self._inner = inner

    def read(self, reader: PacketReader) -> _T | None:
        if reader.read_bool():
            return self._inner.read(reader)
        return None

    def write(self, writer: PacketWriter, value: _T | None) -> None:
        if value is not None:
            writer.write_bool(True)
            self._inner.write(writer, value)
        else:
            writer.write_bool(False)


OPTIONAL_BOOL = OptionalType(BOOL)
OPTIONAL_VEC2 = OptionalType(VEC2)
OPTIONAL_VEC3 = OptionalType(VEC3)
