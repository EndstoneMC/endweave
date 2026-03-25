"""Handlers for LevelSoundEvent remapping - v924 to v944.

v944 added PauseGrowth (597) and ResetGrowth (598) to LevelSoundEvent,
displacing Undefined from 597 to 599. This affects:

- LevelSoundEventPacket (123): Event ID field
- ActorData key 126 (HEARTBEAT_SOUND_EVENT) in:
  SetActorDataPacket (39), AddActorPacket (13),
  AddItemActorPacket (15), AddPlayerPacket (12)
"""

from collections.abc import Callable

from endstone_endweave.codec import (
    ACTOR_DATA_LIST,
    FLOAT_LE,
    ITEM_INSTANCE,
    STRING,
    UUID,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    VEC2,
    VEC3,
    PacketWrapper,
)

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


_INT_REMAPPERS: dict[int, Callable[[int], int]] = {
    _HEARTBEAT_SOUND_EVENT: _remap_sound_clientbound,
}


def _remap_actor_data(wrapper: PacketWrapper) -> None:
    """Read ActorData, remap sound event IDs, write back."""
    entries = wrapper.read(ACTOR_DATA_LIST)
    for entry in entries:
        if entry.key in _INT_REMAPPERS and entry.type_id in (2, 7):
            entry.value = _INT_REMAPPERS[entry.key](entry.value)
    wrapper.write(ACTOR_DATA_LIST, entries)


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
    _remap_actor_data(wrapper)
    wrapper.passthrough_all()  # Synched Properties, Tick


def rewrite_add_actor(wrapper: PacketWrapper) -> None:
    """AddActorPacket (13): entity spawn with ActorData.

    Args:
        wrapper: Packet wrapper for AddActorPacket.
    """
    wrapper.passthrough(VAR_INT64)  # Target Actor ID
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(STRING)  # Actor Type
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC3)  # Velocity
    wrapper.passthrough(VEC2)  # Rotation
    wrapper.passthrough(FLOAT_LE)  # Y Head Rotation
    wrapper.passthrough(FLOAT_LE)  # Y Body Rotation
    attr_count = wrapper.passthrough(UVAR_INT)  # Attributes List
    for _ in range(attr_count):
        wrapper.passthrough(STRING)  # Attribute Name
        wrapper.passthrough(FLOAT_LE)  # Min Value
        wrapper.passthrough(FLOAT_LE)  # Current Value
        wrapper.passthrough(FLOAT_LE)  # Max Value
    _remap_actor_data(wrapper)
    wrapper.passthrough_all()  # Synched Properties, Actor Links


def rewrite_add_item_actor(wrapper: PacketWrapper) -> None:
    """AddItemActorPacket (15): item entity spawn with ActorData.

    Args:
        wrapper: Packet wrapper for AddItemActorPacket.
    """
    wrapper.passthrough(VAR_INT64)  # Target Actor ID
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(ITEM_INSTANCE)  # Item
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC3)  # Velocity
    _remap_actor_data(wrapper)
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
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC3)  # Velocity
    wrapper.passthrough(VEC2)  # Rotation
    wrapper.passthrough(FLOAT_LE)  # Y-Head Rotation
    wrapper.passthrough(ITEM_INSTANCE)  # Carried Item
    wrapper.passthrough(VAR_INT)  # Player Game Type
    _remap_actor_data(wrapper)
    wrapper.passthrough_all()  # Synched Properties, AbilitiesData, Actor Links, Device Id, Build Platform
