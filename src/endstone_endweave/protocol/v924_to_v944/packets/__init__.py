"""Typed packet dataclasses for block-position rewriting.

Each dataclass has decode(reader, read_pos) and encode(writer, write_pos) methods.
The position codec is parameterized by version so the same dataclass handles both
v924 and v944 wire formats.
"""

from endstone_endweave.protocol.v924_to_v944.packets.add_volume_entity import (
    AddVolumeEntityPacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.anvil_damage import (
    AnvilDamagePacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.block_actor_data import (
    BlockActorDataPacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.block_event import (
    BlockEventPacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.container_open import (
    ContainerOpenPacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.lectern_update import (
    LecternUpdatePacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.open_sign import OpenSignPacket
from endstone_endweave.protocol.v924_to_v944.packets.play_sound import PlaySoundPacket
from endstone_endweave.protocol.v924_to_v944.packets.player_action import (
    PlayerActionPacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.set_spawn_position import (
    SetSpawnPositionPacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.structure_template_data_request import (
    StructureSettings,
    StructureTemplateDataRequestPacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.update_block import (
    UpdateBlockPacket,
)
from endstone_endweave.protocol.v924_to_v944.packets.update_block_synced import (
    UpdateBlockSyncedPacket,
)

__all__ = [
    "AddVolumeEntityPacket",
    "AnvilDamagePacket",
    "BlockActorDataPacket",
    "BlockEventPacket",
    "ContainerOpenPacket",
    "LecternUpdatePacket",
    "OpenSignPacket",
    "PlaySoundPacket",
    "PlayerActionPacket",
    "SetSpawnPositionPacket",
    "StructureSettings",
    "StructureTemplateDataRequestPacket",
    "UpdateBlockPacket",
    "UpdateBlockSyncedPacket",
]
