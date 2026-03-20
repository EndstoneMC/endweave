"""Packet field types for read/write/passthrough operations.

Each Type knows how to read from a PacketReader and write to a PacketWriter,
enabling the PacketWrapper's passthrough() pattern (read + write in one call).
Inspired by ViaVersion's Type<T> system.
"""

from abc import ABC, abstractmethod
from typing import Generic, TypeVar

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter

T = TypeVar("T")


class Type(ABC, Generic[T]):
    """A serializable packet field type."""

    @abstractmethod
    def read(self, reader: PacketReader) -> T: ...

    @abstractmethod
    def write(self, writer: PacketWriter, value: T) -> None: ...


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


class _LongLE(Type[int]):
    """Signed 64-bit little-endian integer."""

    def read(self, reader: PacketReader) -> int:
        return reader.read_long_le()

    def write(self, writer: PacketWriter, value: int) -> None:
        writer.write_long_le(value)


class _FloatLE(Type[float]):
    """32-bit little-endian IEEE 754 float."""

    def read(self, reader: PacketReader) -> float:
        return reader.read_float_le()

    def write(self, writer: PacketWriter, value: float) -> None:
        writer.write_float_le(value)


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
        self._length = length

    def read(self, reader: PacketReader) -> bytes:
        return reader.read_bytes(self._length)

    def write(self, writer: PacketWriter, value: bytes) -> None:
        writer.write_bytes(value)


class _CompoundTag(Type[bytes]):
    """Bedrock network NBT CompoundTag -- raw byte passthrough."""

    def read(self, reader: PacketReader) -> bytes:
        start = reader.position
        reader.skip_nbt_compound()
        return reader.slice_from(start)

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
LONG_LE = _LongLE()
FLOAT_LE = _FloatLE()
VAR_INT = _VarInt()
UVAR_INT = _UVarInt()
VAR_INT64 = _VarInt64()
UVAR_INT64 = _UVarInt64()
STRING = _String()
COMPOUND_TAG = _CompoundTag()
REMAINING_BYTES = _RemainingBytes()


class _NetworkBlockPos(Type[tuple[int, int, int]]):
    """v924 NetworkBlockPosition: varint X, uvarint Y, varint Z."""

    def read(self, reader: PacketReader) -> tuple[int, int, int]:
        x = reader.read_varint()
        y = reader.read_uvarint()
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


NETWORK_BLOCK_POS = _NetworkBlockPos()
BLOCK_POS = _BlockPos()


class _ItemInstance(Type[bytes]):
    """Bedrock ItemInstance -- raw byte passthrough.

    Reads through the variable-length ItemInstance structure and returns
    the raw bytes, enabling passthrough without full deserialization.

    Format: varint32 NetworkID (0 = air, done), uint16 Count,
    uvarint32 MetadataValue, bool HasNetID, [varint32 StackNetworkID],
    varint32 BlockRuntimeID, ByteSlice extraData.
    """

    def read(self, reader: PacketReader) -> bytes:
        start = reader.position
        network_id = reader.read_varint()
        if network_id != 0:
            reader.skip(2)  # uint16 Count
            reader.read_uvarint()  # MetadataValue
            has_net_id = reader.read_bool()
            if has_net_id:
                reader.read_varint()  # StackNetworkID
            reader.read_varint()  # BlockRuntimeID
            extra_len = reader.read_uvarint()  # ByteSlice length
            reader.skip(extra_len)
        return reader.slice_from(start)

    def write(self, writer: PacketWriter, value: bytes) -> None:
        writer.write_bytes(value)


ITEM_INSTANCE = _ItemInstance()


def bytes_type(length: int) -> Type[bytes]:
    """Create a fixed-length bytes type."""
    return _Bytes(length)
