"""Handlers for sound remapping in v924 to v898."""

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

_HEARTBEAT_SOUND_EVENT = 127
_OLD_UNDEFINED_SOUND = 578
_DROPPED_ACTOR_DATA_KEYS = {136, 137, 138}


def _remap_sound_clientbound(value: int) -> int:
    if value >= _OLD_UNDEFINED_SOUND:
        return _OLD_UNDEFINED_SOUND
    return value


_INT_REMAPPERS = {_HEARTBEAT_SOUND_EVENT: _remap_sound_clientbound}


def rewrite_level_sound_event(wrapper: PacketWrapper) -> None:
    """LevelSoundEventPacket (123): remap Event ID.

    Args:
        wrapper: Packet wrapper for LevelSoundEventPacket.
    """
    event_id = wrapper.read(UVAR_INT)
    wrapper.write(UVAR_INT, _remap_sound_clientbound(event_id))
    wrapper.passthrough_all()


def rewrite_set_actor_data(wrapper: PacketWrapper) -> None:
    """SetActorDataPacket (39): remap ActorData sound fields.

    Args:
        wrapper: Packet wrapper for SetActorDataPacket.
    """
    wrapper.passthrough(UVAR_INT64)
    passthrough_actor_data(wrapper, _INT_REMAPPERS, _DROPPED_ACTOR_DATA_KEYS)
    wrapper.passthrough_all()


def rewrite_add_actor(wrapper: PacketWrapper) -> None:
    """AddActorPacket (13): entity spawn with ActorData.

    Args:
        wrapper: Packet wrapper for AddActorPacket.
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
    attr_count = wrapper.passthrough(UVAR_INT)
    for _ in range(attr_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(FLOAT_LE)
    passthrough_actor_data(wrapper, _INT_REMAPPERS, _DROPPED_ACTOR_DATA_KEYS)
    wrapper.passthrough_all()


def rewrite_add_item_actor(wrapper: PacketWrapper) -> None:
    """AddItemActorPacket (15): item entity spawn with ActorData.

    Args:
        wrapper: Packet wrapper for AddItemActorPacket.
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
    passthrough_actor_data(wrapper, _INT_REMAPPERS, _DROPPED_ACTOR_DATA_KEYS)
    wrapper.passthrough_all()


def rewrite_add_player(wrapper: PacketWrapper) -> None:
    """AddPlayerPacket (12): player spawn with ActorData.

    Args:
        wrapper: Packet wrapper for AddPlayerPacket.
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
    passthrough_actor_data(wrapper, _INT_REMAPPERS, _DROPPED_ACTOR_DATA_KEYS)
    wrapper.passthrough_all()
