"""Mapping data: v924 (MC 1.26.0) <-> v944 (MC 1.26.10).

LevelSoundEvent -- 2 new IDs inserted at 597 (UNDEFINED_V924):
    597 PAUSE_GROWTH
    598 RESET_GROWTH

NoteBlockInstrument -- 4 new IDs inserted at 16 (TRUMPET):
    16 TRUMPET
    17 TRUMPET (variant)
    18 TRUMPET (variant)
    19 TRUMPET (variant)
    IDs 16-19 are trumpet variants; existing instruments at 16+
    (Zombie, Skeleton, Creeper, WitherSkeleton, Piglin) shift to 20+.
"""

from ...codec.types.enums import ActorDataIDs, LevelSoundEvent, NoteBlockInstrument
from ..mapping_data import MappingData, inserted

MAPPINGS = MappingData(
    sound=inserted(2, at=LevelSoundEvent.UNDEFINED_V924),
    note_instrument=inserted(4, at=NoteBlockInstrument.TRUMPET),
    actor_data_sound_key=ActorDataIDs.HEARTBEAT_SOUND_EVENT,
)
