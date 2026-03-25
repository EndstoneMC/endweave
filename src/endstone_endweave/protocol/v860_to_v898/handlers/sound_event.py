"""Handlers for sound and entity event remapping in v860 to v898."""

from collections.abc import Callable

from endstone_endweave.codec import (
    ACTOR_DATA_LIST,
    BYTE,
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

_ENTITY_EVENT_INSERT_AT = 80
_HEARTBEAT_SOUND_EVENT = 127
_SOUND_INSERT_AT = 566
_SOUND_SHIFT = 12


def _remap_entity_event_clientbound(value: int) -> int:
    if value >= _ENTITY_EVENT_INSERT_AT:
        return value + 1
    return value


def _remap_sound_clientbound(value: int) -> int:
    if value >= _SOUND_INSERT_AT:
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


def rewrite_actor_event(wrapper: PacketWrapper) -> None:
    """Remap EntityEvent ids for v898 clients.

    Args:
        wrapper: Packet wrapper for EntityEvent.
    """
    wrapper.passthrough(UVAR_INT64)
    event_id = wrapper.read(BYTE)
    wrapper.write(BYTE, _remap_entity_event_clientbound(event_id))
    wrapper.passthrough(VAR_INT)


def rewrite_level_sound_event(wrapper: PacketWrapper) -> None:
    """Remap LevelSoundEvent ids for v898 clients.

    Args:
        wrapper: Packet wrapper for LevelSoundEvent.
    """
    event_id = wrapper.read(UVAR_INT)
    wrapper.write(UVAR_INT, _remap_sound_clientbound(event_id))
    wrapper.passthrough_all()


def rewrite_set_actor_data(wrapper: PacketWrapper) -> None:
    """Remap HEARTBEAT_SOUND_EVENT inside ActorData.

    Args:
        wrapper: Packet wrapper for SetActorData.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    _remap_actor_data(wrapper)
    wrapper.passthrough_all()  # Synched Properties, Tick


def rewrite_add_actor(wrapper: PacketWrapper) -> None:
    """Remap HEARTBEAT_SOUND_EVENT for AddActor.

    Args:
        wrapper: Packet wrapper for AddActor.
    """
    wrapper.passthrough(VAR_INT64)  # Target Actor ID
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(STRING)  # Actor Type
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC3)  # Velocity
    wrapper.passthrough(VEC2)  # Rotation
    wrapper.passthrough(FLOAT_LE)  # Y Head Rotation
    wrapper.passthrough(FLOAT_LE)  # Y Body Rotation
    attribute_count = wrapper.passthrough(UVAR_INT)  # Attributes List
    for _ in range(attribute_count):
        wrapper.passthrough(STRING)  # Attribute Name
        wrapper.passthrough(FLOAT_LE)  # Min Value
        wrapper.passthrough(FLOAT_LE)  # Current Value
        wrapper.passthrough(FLOAT_LE)  # Max Value
    _remap_actor_data(wrapper)
    wrapper.passthrough_all()  # Synched Properties, Actor Links


def rewrite_add_item_actor(wrapper: PacketWrapper) -> None:
    """Remap HEARTBEAT_SOUND_EVENT for AddItemActor.

    Args:
        wrapper: Packet wrapper for AddItemActor.
    """
    wrapper.passthrough(VAR_INT64)  # Target Actor ID
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(ITEM_INSTANCE)  # Item
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC3)  # Velocity
    _remap_actor_data(wrapper)
    wrapper.passthrough_all()  # From Fishing?


def rewrite_add_player(wrapper: PacketWrapper) -> None:
    """Remap HEARTBEAT_SOUND_EVENT for AddPlayer.

    Args:
        wrapper: Packet wrapper for AddPlayer.
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
