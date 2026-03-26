"""Shared test helpers for packet construction and verification."""


from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter


def write_net_block_pos(w: PacketWriter, x: int, y: int, z: int) -> None:
    """Write a v924 NetworkBlockPos."""
    w.write_varint(x)
    w.write_uvarint(y)
    w.write_varint(z)


def write_block_pos(w: PacketWriter, x: int, y: int, z: int) -> None:
    """Write a v944 BlockPos."""
    w.write_varint(x)
    w.write_varint(y)
    w.write_varint(z)


def read_block_pos(r: PacketReader) -> tuple[int, int, int]:
    """Read a v944 BlockPos."""
    return (r.read_varint(), r.read_varint(), r.read_varint())


def read_net_block_pos(r: PacketReader) -> tuple[int, int, int]:
    """Read a v924 NetworkBlockPos."""
    return (r.read_varint(), r.read_uvarint(), r.read_varint())


def write_structure_settings_v944(w: PacketWriter) -> None:
    """Write a StructureSettings in v944 format (BlockPos for Size/Offset)."""
    w.write_string("palette")  # PaletteName
    w.write_bool(False)  # IgnoreEntities
    w.write_bool(False)  # IgnoreBlocks
    w.write_bool(True)  # AllowNonTickingChunks
    write_block_pos(w, 10, 20, 30)  # Size (v944 BlockPos)
    write_block_pos(w, -1, 5, -1)  # Offset (v944 BlockPos)
    w.write_varint64(123)  # LastEditingPlayerUniqueID
    w.write_byte(0)  # Rotation
    w.write_byte(0)  # Mirror
    w.write_byte(0)  # AnimationMode
    w.write_float_le(0.0)  # AnimationSeconds
    w.write_float_le(100.0)  # IntegrityValue
    w.write_uint_le(42)  # IntegritySeed
    w.write_float_le(0.5)  # RotationPivot.X
    w.write_float_le(0.5)  # RotationPivot.Y
    w.write_float_le(0.5)  # RotationPivot.Z


def verify_structure_settings_v924(r: PacketReader) -> None:
    """Verify StructureSettings was converted to v924 (NetworkBlockPos for Size/Offset)."""
    assert r.read_string() == "palette"
    assert r.read_bool() is False  # IgnoreEntities
    assert r.read_bool() is False  # IgnoreBlocks
    assert r.read_bool() is True  # AllowNonTickingChunks
    assert read_net_block_pos(r) == (10, 20, 30)  # Size
    assert read_net_block_pos(r) == (-1, 5, -1)  # Offset
    assert r.read_varint64() == 123  # LastEditingPlayerUniqueID
    assert r.read_byte() == 0  # Rotation
    assert r.read_byte() == 0  # Mirror
    assert r.read_byte() == 0  # AnimationMode
    assert r.read_float_le() == 0.0  # AnimationSeconds
    assert r.read_float_le() == 100.0  # IntegrityValue
    assert r.read_uint_le() == 42  # IntegritySeed


def nbt_string(s: str) -> bytes:
    """Encode a uvarint-prefixed UTF-8 string (Bedrock network NBT format)."""
    encoded = s.encode("utf-8")
    w = PacketWriter()
    w.write_uvarint(len(encoded))
    w.write_bytes(encoded)
    return w.to_bytes()


def nbt_varint(val: int) -> bytes:
    """Encode a zigzag varint (Bedrock network NBT format)."""
    w = PacketWriter()
    w.write_varint(val)
    return w.to_bytes()


def nbt_varint64(val: int) -> bytes:
    """Encode a zigzag varint64 (Bedrock network NBT format)."""
    w = PacketWriter()
    w.write_varint64(val)
    return w.to_bytes()


def make_spline_v924(
    total_time: float = 5.0,
    curve_points: list[tuple[float, float, float]] | None = None,
) -> bytes:
    """Build a v924 SplineInstruction in wire format."""
    w = PacketWriter()
    w.write_float_le(total_time)  # totalTime
    w.write_byte(0)  # type
    points = curve_points or [(1.0, 2.0, 3.0)]
    w.write_uvarint(len(points))  # curve count
    for x, y, z in points:
        w.write_float_le(x)
        w.write_float_le(y)
        w.write_float_le(z)
    w.write_uvarint(0)  # progress key frames count
    w.write_uvarint(0)  # rotation key frames count
    return w.to_bytes()


def write_actor_data_list(w: PacketWriter, entries: list[tuple[int, int, bytes]]) -> None:
    """Write an ActorData list from (key, type_id, raw_value_bytes) tuples."""
    w.write_uvarint(len(entries))
    for key, type_id, raw in entries:
        w.write_uvarint(key)
        w.write_uvarint(type_id)
        w.write_bytes(raw)


def varint_bytes(val: int) -> bytes:
    """Encode a signed varint to raw bytes."""
    w = PacketWriter()
    w.write_varint(val)
    return w.to_bytes()
