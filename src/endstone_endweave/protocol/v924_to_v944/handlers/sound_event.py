"""Handlers for LevelSoundEvent remapping -- v924 to v944.

v944 added PauseGrowth (597) and ResetGrowth (598) to LevelSoundEvent,
displacing Undefined from 597 to 599. This affects:

- LevelSoundEventPacket (123): Event ID field
- ActorData key 126 (HEARTBEAT_SOUND_EVENT) in:
  SetActorDataPacket (39), AddActorPacket (13),
  AddItemActorPacket (15), AddPlayerPacket (12)
"""

from endstone_endweave.codec import (
    FLOAT_LE,
    ITEM_INSTANCE,
    STRING,
    UUID,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
)
from endstone_endweave.protocol.rewriter import passthrough_actor_data

# ActorData key for heartbeat sound
_HEARTBEAT_SOUND_EVENT = 126

# v924 Undefined = 597, v944 inserted PauseGrowth(597) + ResetGrowth(598)
_V924_UNDEFINED = 597
_SOUND_SHIFT = 2


def _remap_sound_clientbound(value: int) -> int:
    """Remap LevelSoundEvent from v924 -> v944 (shift Undefined)."""
    if value >= _V924_UNDEFINED:
        return value + _SOUND_SHIFT
    return value


_INT_REMAPPERS = {_HEARTBEAT_SOUND_EVENT: _remap_sound_clientbound}


def rewrite_level_sound_event(wrapper: PacketWrapper) -> None:
    """LevelSoundEventPacket (123): remap Event ID.

    Args:
        wrapper: Packet wrapper for LevelSoundEventPacket.
    """
    event_id = wrapper.read(UVAR_INT)  # Event ID
    wrapper.write(UVAR_INT, _remap_sound_clientbound(event_id))
    wrapper.passthrough_all()  # Position, Data, Actor Identifier, Is Baby, Is Global, Actor Unique Id


def rewrite_set_actor_data(wrapper: PacketWrapper) -> None:
    """SetActorDataPacket (39): RuntimeID + ActorData + trailing fields.

    Args:
        wrapper: Packet wrapper for SetActorDataPacket.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    passthrough_actor_data(wrapper, _INT_REMAPPERS)
    wrapper.passthrough_all()  # Synched Properties, Tick


def rewrite_add_actor(wrapper: PacketWrapper) -> None:
    """AddActorPacket (13): entity spawn with ActorData.

    Args:
        wrapper: Packet wrapper for AddActorPacket.
    """
    wrapper.passthrough(VAR_INT64)  # Target Actor ID
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(STRING)  # Actor Type
    wrapper.passthrough(FLOAT_LE)  # Position.X
    wrapper.passthrough(FLOAT_LE)  # Position.Y
    wrapper.passthrough(FLOAT_LE)  # Position.Z
    wrapper.passthrough(FLOAT_LE)  # Velocity.X
    wrapper.passthrough(FLOAT_LE)  # Velocity.Y
    wrapper.passthrough(FLOAT_LE)  # Velocity.Z
    wrapper.passthrough(FLOAT_LE)  # Rotation.X
    wrapper.passthrough(FLOAT_LE)  # Rotation.Y
    wrapper.passthrough(FLOAT_LE)  # Y Head Rotation
    wrapper.passthrough(FLOAT_LE)  # Y Body Rotation
    attr_count = wrapper.passthrough(UVAR_INT)  # Attributes List
    for _ in range(attr_count):
        wrapper.passthrough(STRING)  # Attribute Name
        wrapper.passthrough(FLOAT_LE)  # Min Value
        wrapper.passthrough(FLOAT_LE)  # Current Value
        wrapper.passthrough(FLOAT_LE)  # Max Value
    passthrough_actor_data(wrapper, _INT_REMAPPERS)
    wrapper.passthrough_all()  # Synched Properties, Actor Links


def rewrite_add_item_actor(wrapper: PacketWrapper) -> None:
    """AddItemActorPacket (15): item entity spawn with ActorData.

    Args:
        wrapper: Packet wrapper for AddItemActorPacket.
    """
    wrapper.passthrough(VAR_INT64)  # Target Actor ID
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(ITEM_INSTANCE)  # Item
    wrapper.passthrough(FLOAT_LE)  # Position.X
    wrapper.passthrough(FLOAT_LE)  # Position.Y
    wrapper.passthrough(FLOAT_LE)  # Position.Z
    wrapper.passthrough(FLOAT_LE)  # Velocity.X
    wrapper.passthrough(FLOAT_LE)  # Velocity.Y
    wrapper.passthrough(FLOAT_LE)  # Velocity.Z
    passthrough_actor_data(wrapper, _INT_REMAPPERS)
    wrapper.passthrough_all()  # From Fishing?


def rewrite_add_player(wrapper: PacketWrapper) -> None:
    """AddPlayerPacket (12): player spawn with ActorData.

    Args:
        wrapper: Packet wrapper for AddPlayerPacket.
    """
    wrapper.passthrough(UUID)  # UUID
    wrapper.passthrough(STRING)  # Player Name
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(STRING)  # Platform Chat Id
    wrapper.passthrough(FLOAT_LE)  # Position.X
    wrapper.passthrough(FLOAT_LE)  # Position.Y
    wrapper.passthrough(FLOAT_LE)  # Position.Z
    wrapper.passthrough(FLOAT_LE)  # Velocity.X
    wrapper.passthrough(FLOAT_LE)  # Velocity.Y
    wrapper.passthrough(FLOAT_LE)  # Velocity.Z
    wrapper.passthrough(FLOAT_LE)  # Rotation.X
    wrapper.passthrough(FLOAT_LE)  # Rotation.Y
    wrapper.passthrough(FLOAT_LE)  # Y-Head Rotation
    wrapper.passthrough(ITEM_INSTANCE)  # Carried Item
    wrapper.passthrough(VAR_INT)  # Player Game Type
    passthrough_actor_data(wrapper, _INT_REMAPPERS)
    wrapper.passthrough_all()  # Synched Properties, AbilitiesData, Actor Links, Device Id, Build Platform
