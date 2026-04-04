"""Mapping data: v924 (MC 1.26.0) <-> v944 (MC 1.26.10).

LevelSoundEvent -- 2 new IDs inserted at 597 (UNDEFINED_V924):
    597 PAUSE_GROWTH
    598 RESET_GROWTH

NoteBlockInstrument -- 4 new IDs inserted at 16:
    16 TRUMPET
    17 TRUMPET (variant)
    18 TRUMPET (variant)
    19 TRUMPET (variant)
    IDs 16-19 are trumpet variants; existing instruments at 16+
    (Zombie, Skeleton, Creeper, WitherSkeleton, Piglin) shift to 20+.
"""

from endstone_endweave.codec.types.enums import ActorDataIDs, LevelSoundEvent
from endstone_endweave.protocol.mapping_data import IdShift, MappingData

MAPPINGS = MappingData(
    sound=IdShift(LevelSoundEvent.UNDEFINED_V924, LevelSoundEvent.UNDEFINED_V944 - LevelSoundEvent.UNDEFINED_V924),
    note_instrument=IdShift(16, 4),
    actor_data_sound_key=ActorDataIDs.HEARTBEAT_SOUND_EVENT,
)
