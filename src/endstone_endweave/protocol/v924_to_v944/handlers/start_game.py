"""Handler for StartGamePacket (11) - clientbound."""

from __future__ import annotations

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.session import PlayerSession
from endstone_endweave.protocol.base import PacketTransformation
from endstone_endweave.protocol.v924_to_v944.block_position import convert_924_to_944


def rewrite_start_game(
    payload: bytes, session: PlayerSession
) -> PacketTransformation:
    """Upscale v924 -> v944: convert NetworkBlockPosition to BlockPos in LevelSettings."""
    reader = PacketReader(payload)
    writer = PacketWriter()

    # 1. Entity ID (ActorUniqueID = signed varlong)
    writer.write_varlong(reader.read_varlong())
    # 2. Runtime ID (ActorRuntimeID = unsigned varlong)
    writer.write_varlong(reader.read_varlong())
    # 3. Game Type (varint)
    writer.write_signed_varint(reader.read_signed_varint())
    # 4. Position (Vec3 = 3x float LE = 12 bytes)
    writer.write_bytes(reader.read_bytes(12))
    # 5. Rotation (Vec2 = 2x float LE = 8 bytes)
    writer.write_bytes(reader.read_bytes(8))

    # --- Begin LevelSettings ---
    # 6. Seed (uint64 LE = 8 bytes)
    writer.write_bytes(reader.read_bytes(8))

    # --- Begin SpawnSettings ---
    # 7. Generator Type (varint)
    writer.write_signed_varint(reader.read_signed_varint())
    # 8. Game Type (varint)
    writer.write_signed_varint(reader.read_signed_varint())
    # 9. is Hardcore Mode enabled? (bool)
    writer.write_bool(reader.read_bool())
    # 10. Game Difficulty (varint)
    writer.write_signed_varint(reader.read_signed_varint())

    # 11. Default Spawn Block Position - THE CONVERSION
    convert_924_to_944(reader, writer)

    # Copy everything after verbatim (rest of LevelSettings + rest of StartGamePacket)
    writer.write_bytes(reader.read_remaining())

    return PacketTransformation(new_payload=writer.to_bytes())
