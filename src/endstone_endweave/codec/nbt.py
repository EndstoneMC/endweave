"""Bedrock little-endian NBT compound skipper.

Advances a PacketReader past a complete NBT compound tag without
interpreting the values. Only the structure is parsed to find the end.
"""

from __future__ import annotations

from endstone_endweave.codec.reader import PacketReader

# NBT tag type IDs
_END = 0
_BYTE = 1
_SHORT = 2
_INT = 3
_LONG = 4
_FLOAT = 5
_DOUBLE = 6
_BYTE_ARRAY = 7
_STRING = 8
_LIST = 9
_COMPOUND = 10
_INT_ARRAY = 11
_LONG_ARRAY = 12

_FIXED_SIZES = {
    _BYTE: 1,
    _SHORT: 2,
    _INT: 4,
    _LONG: 8,
    _FLOAT: 4,
    _DOUBLE: 8,
}


def skip_nbt_compound(reader: PacketReader) -> None:
    """Skip a Bedrock LE NBT compound tag (including its outer tag header).

    Expects the reader to be positioned at the compound's tag-type byte (0x0A).
    """
    tag_type = reader.read_byte()
    if tag_type != _COMPOUND:
        raise ValueError(f"Expected compound tag (10), got {tag_type}")
    # Root compound name (LE short-prefixed string)
    _skip_nbt_string(reader)
    _skip_compound_payload(reader)


def _skip_nbt_string(reader: PacketReader) -> None:
    """Skip a short-LE-prefixed UTF-8 string."""
    length = reader.read_ushort_le()
    reader.skip(length)


def _skip_tag_payload(reader: PacketReader, tag_type: int) -> None:
    """Skip the payload of a single tag (not including its type byte or name)."""
    fixed = _FIXED_SIZES.get(tag_type)
    if fixed is not None:
        reader.skip(fixed)
        return

    if tag_type == _STRING:
        _skip_nbt_string(reader)
    elif tag_type == _BYTE_ARRAY:
        length = reader.read_int_le()
        reader.skip(length)
    elif tag_type == _INT_ARRAY:
        length = reader.read_int_le()
        reader.skip(length * 4)
    elif tag_type == _LONG_ARRAY:
        length = reader.read_int_le()
        reader.skip(length * 8)
    elif tag_type == _LIST:
        element_type = reader.read_byte()
        count = reader.read_int_le()
        for _ in range(count):
            _skip_tag_payload(reader, element_type)
    elif tag_type == _COMPOUND:
        _skip_compound_payload(reader)
    else:
        raise ValueError(f"Unknown NBT tag type: {tag_type}")


def _skip_compound_payload(reader: PacketReader) -> None:
    """Skip the body of a compound tag (the sequence of named tags until End)."""
    while True:
        child_type = reader.read_byte()
        if child_type == _END:
            return
        _skip_nbt_string(reader)
        _skip_tag_payload(reader, child_type)
