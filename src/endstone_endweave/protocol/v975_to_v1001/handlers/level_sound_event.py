"""LevelSoundEvent(123) v975 server <-> v1001 client.

v1001 changed Sound ID to String
"""

from endstone_endweave.codec import STRING, UVAR_INT, PacketWrapper
from endstone_endweave.protocol.direction import Direction

from .sound_event_map import SOUND_EVENT_IDS, SOUND_EVENT_NAMES


def rewrite_level_sound_event(wrapper: PacketWrapper, direction: Direction) -> None:
    """LevelSoundEvent (123): rewrite v1001 wire string back to numeric SoundType.

    Args:
        wrapper: Packet wrapper for a LevelSoundEventPacket.
        direction: Whether the packet is clientbound or serverbound.
    """

    if direction is Direction.CLIENTBOUND:
        sound_id = wrapper.read(UVAR_INT)  # v975 numeric SoundType (enum id)
        wrapper.write(STRING, SOUND_EVENT_NAMES.get(sound_id, ""))
        wrapper.passthrough_all()
    else:
        sound_name = wrapper.read(STRING)  # v1001 String SoundType
        wrapper.write(UVAR_INT, SOUND_EVENT_IDS.get(sound_name, 0))
        wrapper.passthrough_all()
