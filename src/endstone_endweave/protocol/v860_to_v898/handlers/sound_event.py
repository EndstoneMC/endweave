"""Handlers for sound and entity event remapping in v860 to v898."""

from endstone_endweave.codec import (
    BYTE,
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


_INT_REMAPPERS = {_HEARTBEAT_SOUND_EVENT: _remap_sound_clientbound}


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
    wrapper.passthrough(UVAR_INT64)
    passthrough_actor_data(wrapper, _INT_REMAPPERS)
    wrapper.passthrough_all()


def rewrite_add_actor(wrapper: PacketWrapper) -> None:
    """Remap HEARTBEAT_SOUND_EVENT for AddActor.

    Args:
        wrapper: Packet wrapper for AddActor.
    """
    wrapper.passthrough(VAR_INT64)
    wrapper.passthrough(UVAR_INT64)
    wrapper.passthrough(STRING)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    attribute_count = wrapper.passthrough(UVAR_INT)
    for _ in range(attribute_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(FLOAT_LE)
    passthrough_actor_data(wrapper, _INT_REMAPPERS)
    wrapper.passthrough_all()


def rewrite_add_item_actor(wrapper: PacketWrapper) -> None:
    """Remap HEARTBEAT_SOUND_EVENT for AddItemActor.

    Args:
        wrapper: Packet wrapper for AddItemActor.
    """
    wrapper.passthrough(VAR_INT64)
    wrapper.passthrough(UVAR_INT64)
    wrapper.passthrough(ITEM_INSTANCE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    passthrough_actor_data(wrapper, _INT_REMAPPERS)
    wrapper.passthrough_all()


def rewrite_add_player(wrapper: PacketWrapper) -> None:
    """Remap HEARTBEAT_SOUND_EVENT for AddPlayer.

    Args:
        wrapper: Packet wrapper for AddPlayer.
    """
    wrapper.passthrough(UUID)
    wrapper.passthrough(STRING)
    wrapper.passthrough(UVAR_INT64)
    wrapper.passthrough(STRING)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(ITEM_INSTANCE)
    wrapper.passthrough(VAR_INT)
    passthrough_actor_data(wrapper, _INT_REMAPPERS)
    wrapper.passthrough_all()
