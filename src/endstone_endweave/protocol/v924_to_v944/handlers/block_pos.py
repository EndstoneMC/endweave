"""Typed block-position packet handlers using closure factories."""

from __future__ import annotations

from typing import Any

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.session import PlayerSession
from endstone_endweave.protocol.base import PacketTransformation, ProtocolTranslator
from endstone_endweave.protocol.v924_to_v944.block_position import (
    read_pos_v924,
    read_pos_v944,
    write_pos_v924,
    write_pos_v944,
)
from endstone_endweave.protocol.v924_to_v944.packet_ids import PacketId
from endstone_endweave.protocol.v924_to_v944.packets import (
    AddVolumeEntityPacket,
    AnvilDamagePacket,
    BlockActorDataPacket,
    BlockEventPacket,
    ContainerOpenPacket,
    LecternUpdatePacket,
    OpenSignPacket,
    PlayerActionPacket,
    PlaySoundPacket,
    SetSpawnPositionPacket,
    StructureTemplateDataRequestPacket,
    UpdateBlockPacket,
    UpdateBlockSyncedPacket,
)

_CLIENTBOUND: list[tuple[int, type]] = [
    (PacketId.UPDATE_BLOCK, UpdateBlockPacket),
    (PacketId.TILE_EVENT, BlockEventPacket),
    (PacketId.SET_SPAWN_POSITION, SetSpawnPositionPacket),
    (PacketId.CONTAINER_OPEN, ContainerOpenPacket),
    (PacketId.BLOCK_ACTOR_DATA, BlockActorDataPacket),
    (PacketId.PLAY_SOUND, PlaySoundPacket),
    (PacketId.UPDATE_BLOCK_SYNCED, UpdateBlockSyncedPacket),
    (PacketId.ADD_VOLUME_ENTITY, AddVolumeEntityPacket),
    (PacketId.OPEN_SIGN, OpenSignPacket),
]

_SERVERBOUND: list[tuple[int, type]] = [
    (PacketId.PLAYER_ACTION, PlayerActionPacket),
    (PacketId.LECTERN_UPDATE, LecternUpdatePacket),
    (PacketId.STRUCTURE_TEMPLATE_DATA_EXPORT_REQUEST, StructureTemplateDataRequestPacket),
    (PacketId.ANVIL_DAMAGE, AnvilDamagePacket),
]


def _make_cb_handler(packet_cls: Any):
    """Create a clientbound handler: read v924 positions, write v944."""
    def handler(payload: bytes, session: PlayerSession) -> PacketTransformation:
        reader = PacketReader(payload)
        packet = packet_cls.decode(reader, read_pos_v924)
        writer = PacketWriter()
        packet.encode(writer, write_pos_v944)
        return PacketTransformation(new_payload=writer.to_bytes())
    return handler


def _make_sb_handler(packet_cls: Any):
    """Create a serverbound handler: read v944 positions, write v924."""
    def handler(payload: bytes, session: PlayerSession) -> PacketTransformation:
        reader = PacketReader(payload)
        packet = packet_cls.decode(reader, read_pos_v944)
        writer = PacketWriter()
        packet.encode(writer, write_pos_v924)
        return PacketTransformation(new_payload=writer.to_bytes())
    return handler


def register_block_pos_handlers(translator: ProtocolTranslator) -> None:
    """Register all typed block-position packet handlers."""
    for packet_id, packet_cls in _CLIENTBOUND:
        translator.register_clientbound(packet_id, _make_cb_handler(packet_cls))
    for packet_id, packet_cls in _SERVERBOUND:
        translator.register_serverbound(packet_id, _make_sb_handler(packet_cls))
