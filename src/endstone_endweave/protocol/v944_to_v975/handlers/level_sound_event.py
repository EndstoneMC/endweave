"""LevelSoundEventPacket (123) -- v944 server to v975 client.

Remaps the Event ID (shifts new v975 sound IDs up) and appends the optional
Fire At Position field added in v975.
"""

from endstone_endweave.codec import BOOL, INT64_LE, STRING, UVAR_INT, VAR_INT, VEC3, PacketWrapper
from endstone_endweave.protocol.mappings.v944_v975 import MAPPINGS


def rewrite_level_sound_event(wrapper: PacketWrapper) -> None:
    """Remap Event ID and append Fire At Position.

    Args:
        wrapper: Packet wrapper for LevelSoundEventPacket.
    """
    event_id = wrapper.read(UVAR_INT)  # Event ID
    wrapper.write(UVAR_INT, MAPPINGS.sound.shift_up(event_id))  # Event ID
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VAR_INT)  # Data
    wrapper.passthrough(STRING)  # Actor Identifier
    wrapper.passthrough(BOOL)  # Is Baby
    wrapper.passthrough(BOOL)  # Is Global
    wrapper.passthrough(INT64_LE)  # Actor Unique Id
    wrapper.write(BOOL, False)  # Fire At Position (not present)
