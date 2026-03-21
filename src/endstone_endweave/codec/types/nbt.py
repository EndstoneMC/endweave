"""Bedrock network NBT tag hierarchy and codec types.

Bedrock network NBT differs from Java Edition:
- String lengths use uvarint (not big-endian short)
- Int values use zigzag varint (not fixed 4 bytes big-endian)
- Int64 values use zigzag varint64 (not fixed 8 bytes big-endian)
- Shorts, floats, and doubles are little-endian (not big-endian)
"""

import struct
from abc import ABC, abstractmethod
from dataclasses import dataclass, field

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.types.primitives import Type
from endstone_endweave.codec.writer import PacketWriter

_MAX_DEPTH = 512

TAG_END = 0
TAG_BYTE = 1
TAG_SHORT = 2
TAG_INT = 3
TAG_LONG = 4
TAG_FLOAT = 5
TAG_DOUBLE = 6
TAG_BYTE_ARRAY = 7
TAG_STRING = 8
TAG_LIST = 9
TAG_COMPOUND = 10
TAG_INT_ARRAY = 11
TAG_LONG_ARRAY = 12


class Tag(ABC):
    """Base class for all NBT tags."""

    @abstractmethod
    def tag_id(self) -> int:
        """Return the numeric tag type ID."""
        ...


@dataclass
class ByteTag(Tag):
    """NBT byte tag (ID=1)."""

    value: int = 0

    def tag_id(self) -> int:
        return TAG_BYTE


@dataclass
class ShortTag(Tag):
    """NBT short tag (ID=2), signed 16-bit LE."""

    value: int = 0

    def tag_id(self) -> int:
        return TAG_SHORT


@dataclass
class IntTag(Tag):
    """NBT int tag (ID=3), zigzag varint."""

    value: int = 0

    def tag_id(self) -> int:
        return TAG_INT


@dataclass
class LongTag(Tag):
    """NBT long tag (ID=4), zigzag varint64."""

    value: int = 0

    def tag_id(self) -> int:
        return TAG_LONG


@dataclass
class FloatTag(Tag):
    """NBT float tag (ID=5), 32-bit LE IEEE 754."""

    value: float = 0.0

    def tag_id(self) -> int:
        return TAG_FLOAT


@dataclass
class DoubleTag(Tag):
    """NBT double tag (ID=6), 64-bit LE IEEE 754."""

    value: float = 0.0

    def tag_id(self) -> int:
        return TAG_DOUBLE


@dataclass
class ByteArrayTag(Tag):
    """NBT byte array tag (ID=7)."""

    value: bytes = b""

    def tag_id(self) -> int:
        return TAG_BYTE_ARRAY


@dataclass
class StringTag(Tag):
    """NBT string tag (ID=8), uvarint-prefixed UTF-8."""

    value: str = ""

    def tag_id(self) -> int:
        return TAG_STRING


@dataclass
class ListTag(Tag):
    """NBT list tag (ID=9), homogeneous list of tags.

    Attributes:
        element_type: Tag type ID of list elements.
        tags: The list of child tags.
    """

    element_type: int = TAG_END
    tags: list[Tag] = field(default_factory=list)

    def tag_id(self) -> int:
        return TAG_LIST

    def __getitem__(self, index: int) -> Tag:
        return self.tags[index]

    def __len__(self) -> int:
        return len(self.tags)


@dataclass
class CompoundTag(Tag):
    """NBT compound tag (ID=10), ordered string-to-Tag mapping.

    Supports dict-like access via ``tag["key"]``, ``tag["key"] = value``,
    ``del tag["key"]``, and ``"key" in tag``.

    Attributes:
        value: The underlying dict of name-to-tag entries.
    """

    value: dict[str, Tag] = field(default_factory=dict)

    def tag_id(self) -> int:
        return TAG_COMPOUND

    def __getitem__(self, key: str) -> Tag:
        return self.value[key]

    def __setitem__(self, key: str, tag: Tag) -> None:
        self.value[key] = tag

    def __delitem__(self, key: str) -> None:
        del self.value[key]

    def __contains__(self, key: object) -> bool:
        return key in self.value

    def get(self, key: str, default: Tag | None = None) -> Tag | None:
        """Get a tag by key, returning default if not found."""
        return self.value.get(key, default)


@dataclass
class IntArrayTag(Tag):
    """NBT int array tag (ID=11), varint count + varint elements."""

    value: list[int] = field(default_factory=list)

    def tag_id(self) -> int:
        return TAG_INT_ARRAY


@dataclass
class LongArrayTag(Tag):
    """NBT long array tag (ID=12), varint count + varint64 elements."""

    value: list[int] = field(default_factory=list)

    def tag_id(self) -> int:
        return TAG_LONG_ARRAY


# ---------------------------------------------------------------------------
# Bedrock network NBT string helpers
# ---------------------------------------------------------------------------


def _read_nbt_string(reader: PacketReader) -> str:
    """Read a uvarint-prefixed UTF-8 string."""
    length = reader.read_uvarint()
    return reader.read_bytes(length).decode("utf-8")


def _write_nbt_string(writer: PacketWriter, value: str) -> None:
    """Write a uvarint-prefixed UTF-8 string."""
    encoded = value.encode("utf-8")
    writer.write_uvarint(len(encoded))
    writer.write_bytes(encoded)


# ---------------------------------------------------------------------------
# Read
# ---------------------------------------------------------------------------


def _read_value(reader: PacketReader, tag_type: int, depth: int) -> Tag:
    """Read a tag value (without header) given its type ID.

    Args:
        reader: The packet reader.
        tag_type: The tag type ID.
        depth: Current nesting depth for overflow protection.

    Returns:
        The deserialized Tag.

    Raises:
        ValueError: If nesting depth exceeds 512 or tag type is unknown.
    """
    if depth > _MAX_DEPTH:
        raise ValueError("NBT nesting depth exceeded")

    if tag_type == TAG_BYTE:
        return ByteTag(reader.read_byte())
    if tag_type == TAG_SHORT:
        return ShortTag(reader.read_short_le())
    if tag_type == TAG_INT:
        return IntTag(reader.read_varint())
    if tag_type == TAG_LONG:
        return LongTag(reader.read_varint64())
    if tag_type == TAG_FLOAT:
        return FloatTag(reader.read_float_le())
    if tag_type == TAG_DOUBLE:
        return DoubleTag(struct.unpack("<d", reader.read_bytes(8))[0])
    if tag_type == TAG_BYTE_ARRAY:
        length = reader.read_varint()
        return ByteArrayTag(reader.read_bytes(length))
    if tag_type == TAG_STRING:
        return StringTag(_read_nbt_string(reader))
    if tag_type == TAG_LIST:
        elem_type = reader.read_byte()
        count = reader.read_varint()
        tags: list[Tag] = []
        if elem_type != TAG_END and count > 0:
            for _ in range(count):
                tags.append(_read_value(reader, elem_type, depth + 1))
        return ListTag(elem_type, tags)
    if tag_type == TAG_COMPOUND:
        entries: dict[str, Tag] = {}
        while True:
            child_type = reader.read_byte()
            if child_type == TAG_END:
                break
            name = _read_nbt_string(reader)
            entries[name] = _read_value(reader, child_type, depth + 1)
        return CompoundTag(entries)
    if tag_type == TAG_INT_ARRAY:
        count = reader.read_varint()
        return IntArrayTag([reader.read_varint() for _ in range(count)])
    if tag_type == TAG_LONG_ARRAY:
        count = reader.read_varint()
        return LongArrayTag([reader.read_varint64() for _ in range(count)])
    raise ValueError(f"Unknown NBT tag type: {tag_type}")


def read_nbt(reader: PacketReader, read_name: bool = True) -> CompoundTag | None:
    """Read a network NBT root compound tag.

    A root type of 0 (End) returns None (absent NBT).

    Args:
        reader: The packet reader.
        read_name: If True, read and discard the root name string
            (Bedrock named format). If False, skip the name
            (Java 1.20.2+ nameless format).

    Returns:
        The deserialized root CompoundTag, or None if the root tag is End.

    Raises:
        ValueError: If the root tag type is not compound (10) or end (0).

    See Also:
        com.viaversion.viaversion.api.type.types.misc.NamedCompoundTagType#read
    """
    root_type = reader.read_byte()
    if root_type == TAG_END:
        return None
    if root_type != TAG_COMPOUND:
        raise ValueError(f"Expected compound root tag (10), got {root_type}")
    if read_name:
        _read_nbt_string(reader)  # root name (usually empty)
    entries: dict[str, Tag] = {}
    while True:
        child_type = reader.read_byte()
        if child_type == TAG_END:
            break
        name = _read_nbt_string(reader)
        entries[name] = _read_value(reader, child_type, 1)
    return CompoundTag(entries)


# ---------------------------------------------------------------------------
# Write
# ---------------------------------------------------------------------------


def _write_value(writer: PacketWriter, tag: Tag) -> None:
    """Write a tag value (without header)."""
    if isinstance(tag, ByteTag):
        writer.write_byte(tag.value)
    elif isinstance(tag, ShortTag):
        writer.write_short_le(tag.value)
    elif isinstance(tag, IntTag):
        writer.write_varint(tag.value)
    elif isinstance(tag, LongTag):
        writer.write_varint64(tag.value)
    elif isinstance(tag, FloatTag):
        writer.write_float_le(tag.value)
    elif isinstance(tag, DoubleTag):
        writer.write_bytes(struct.pack("<d", tag.value))
    elif isinstance(tag, ByteArrayTag):
        writer.write_varint(len(tag.value))
        writer.write_bytes(tag.value)
    elif isinstance(tag, StringTag):
        _write_nbt_string(writer, tag.value)
    elif isinstance(tag, ListTag):
        writer.write_byte(tag.element_type)
        writer.write_varint(len(tag.tags))
        for child in tag.tags:
            _write_value(writer, child)
    elif isinstance(tag, CompoundTag):
        for name, child in tag.value.items():
            writer.write_byte(child.tag_id())
            _write_nbt_string(writer, name)
            _write_value(writer, child)
        writer.write_byte(TAG_END)
    elif isinstance(tag, IntArrayTag):
        writer.write_varint(len(tag.value))
        for v in tag.value:
            writer.write_varint(v)
    elif isinstance(tag, LongArrayTag):
        writer.write_varint(len(tag.value))
        for v in tag.value:
            writer.write_varint64(v)
    else:
        raise ValueError(f"Unknown tag type: {type(tag).__name__}")


def write_nbt(writer: PacketWriter, tag: CompoundTag | None, name: str | None = "") -> None:
    """Write a network NBT root compound tag.

    None is written as a single End byte (0).

    Args:
        writer: The packet writer.
        tag: The root CompoundTag to serialize, or None for absent NBT.
        name: Root name to write. Pass ``""`` for Bedrock named format,
            or ``None`` to omit the name (Java 1.20.2+ nameless format).

    See Also:
        com.viaversion.viaversion.api.type.types.misc.NamedCompoundTagType#write
    """
    if tag is None:
        writer.write_byte(TAG_END)
        return
    writer.write_byte(TAG_COMPOUND)
    if name is not None:
        _write_nbt_string(writer, name)
    _write_value(writer, tag)


# ---------------------------------------------------------------------------
# Type wrappers for PacketWrapper.passthrough()
# ---------------------------------------------------------------------------


class NamedCompoundTagType(Type[CompoundTag | None]):
    """NBT CompoundTag with root name prefix (Bedrock network format).

    Returns None for an absent root (TAG_END), and a CompoundTag for a
    present root.

    See Also:
        com.viaversion.viaversion.api.type.types.misc.NamedCompoundTagType
    """

    def read(self, reader: PacketReader) -> CompoundTag | None:
        return read_nbt(reader, read_name=True)

    def write(self, writer: PacketWriter, value: CompoundTag | None) -> None:
        write_nbt(writer, value, name="")


class CompoundTagType(Type[CompoundTag | None]):
    """NBT CompoundTag without root name prefix (Java 1.20.2+ format).

    Delegates to ``read_nbt``/``write_nbt`` with name handling disabled.

    See Also:
        com.viaversion.viaversion.api.type.types.misc.CompoundTagType
    """

    def read(self, reader: PacketReader) -> CompoundTag | None:
        return read_nbt(reader, read_name=False)

    def write(self, writer: PacketWriter, value: CompoundTag | None) -> None:
        write_nbt(writer, value, name=None)


NAMED_COMPOUND_TAG = NamedCompoundTagType()
COMPOUND_TAG = CompoundTagType()
