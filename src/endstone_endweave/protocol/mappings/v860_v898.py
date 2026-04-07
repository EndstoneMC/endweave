"""Mapping data: v860 (MC 1.21.124) <-> v898 (MC 1.21.130).

LevelSoundEvent -- 12 new IDs inserted at 566 (UNDEFINED_V860):
    566 LUNGE1                  570 SPEAR_ATTACK_MISS
    567 LUNGE2                  571 WOODEN_SPEAR_ATTACK_HIT
    568 LUNGE3                  572 WOODEN_SPEAR_ATTACK_MISS
    569 ATTACK_CRITICAL          573 IMITATE_PARCHED
    570 SPEAR_ATTACK_HIT         574 IMITATE_CAMEL_HUSK
                                575 SPEAR_USE
                                576 WOODEN_SPEAR_USE

ActorEvent -- 1 new ID inserted at 80:
    80  KINETIC_DAMAGE_DEALT
"""

from ...codec.types.enums import ActorDataIDs, ActorEvent, LevelSoundEvent
from ..mapping_data import MappingData, inserted

MAPPINGS = MappingData(
    sound=inserted(12, at=LevelSoundEvent.UNDEFINED_V860),
    actor_event=inserted(1, at=ActorEvent.KINETIC_DAMAGE_DEALT),
    actor_data_sound_key=ActorDataIDs.HEARTBEAT_SOUND_EVENT,
)
