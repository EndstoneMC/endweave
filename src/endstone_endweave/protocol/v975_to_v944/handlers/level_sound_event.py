"""LevelSoundEventPacket (123) -- v975 server to v944 client.

Remaps the Event ID (collapses new v975 sound IDs to v944's UNDEFINED)
and strips the optional Fire At Position field added in v975.
"""

from ....codec import BOOL, INT64_LE, OPTIONAL_VEC3, STRING, UVAR_INT, VAR_INT, VEC3, PacketWrapper
from ...mappings.v944_v975 import MAPPINGS


def rewrite_level_sound_event(wrapper: PacketWrapper) -> None:
    """Remap Event ID and strip Fire At Position.

    Args:
        wrapper: Packet wrapper for LevelSoundEventPacket.
    """
    event_id = wrapper.read(UVAR_INT)  # Event ID
    wrapper.write(UVAR_INT, MAPPINGS.sound.shift_down(event_id))  # Event ID
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VAR_INT)  # Data
    wrapper.passthrough(STRING)  # Actor Identifier
    wrapper.passthrough(BOOL)  # Is Baby
    wrapper.passthrough(BOOL)  # Is Global
    wrapper.passthrough(INT64_LE)  # Actor Unique Id
    wrapper.read(OPTIONAL_VEC3)  # Fire At Position (strip)
