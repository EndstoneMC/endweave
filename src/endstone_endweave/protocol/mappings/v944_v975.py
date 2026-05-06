"""Mapping data: v944 (MC 1.26.10) <-> v975 (MC 1.26.20).

LevelSoundEvent -- 2 new IDs inserted at 599 (UNDEFINED_V944):
    599 PUSHED_BY_PLAYER
    600 BOUNCE
"""

from endstone_endweave.codec.types.enums import ActorDataIDs, LevelSoundEvent
from endstone_endweave.protocol.mapping_data import MappingData, inserted

MAPPINGS = MappingData(
    sound=inserted(2, at=LevelSoundEvent.UNDEFINED_V944),
    actor_data_sound_key=ActorDataIDs.HEARTBEAT_SOUND_EVENT,
)
