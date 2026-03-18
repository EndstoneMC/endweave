"""Handler for UpdateClientInputLocksPacket (196) - clientbound.

v924 sends varuint32 + Vec3(3x float LE); v944 expects varuint32 only.
Strip the trailing 12-byte Vec3.
"""

from __future__ import annotations

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.base import PacketTransformation
from endstone_endweave.session import PlayerSession


def rewrite_client_input_locks(
    payload: bytes, session: PlayerSession
) -> PacketTransformation:
    """Strip Vec3 from v924 server -> v944 client."""
    reader = PacketReader(payload)
    writer = PacketWriter()

    # Lock flags (unsigned varint) — keep
    writer.write_varint(reader.read_varint())

    # Discard Vec3 (3x float LE, 12 bytes) present in v924 but not v944

    return PacketTransformation(new_payload=writer.to_bytes())
