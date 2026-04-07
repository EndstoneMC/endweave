"""Reusable sound and ActorData remapping for protocol translation.

Centralizes the boilerplate for LevelSoundEvent ID remapping and
ActorData integer field remapping (e.g. heartbeat sound event key).
Each version pair creates a SoundRewriter with its remapping config
and calls register() on the Protocol.

See Also:
    com.viaversion.viaversion.rewriter.SoundRewriter
"""

from collections.abc import Callable, Set

from ..codec import (
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
    DataItemType,
    PacketWrapper,
)
from ..protocol import Protocol
from ..protocol.packet_ids import PacketId


class SoundRewriter:
    """Registers clientbound handlers for sound event and ActorData remapping.

    Args:
        sound_remap: Function mapping old sound event ID to new ID.
        actor_data_int_remappers: Per-key remap functions for ActorData
            integer fields (type_id 2 or 7).
        dropped_actor_data_keys: ActorData keys to filter out entirely.

    See Also:
        com.viaversion.viaversion.rewriter.SoundRewriter
    """

    def __init__(
        self,
        sound_remap: Callable[[int], int],
        actor_data_int_remappers: dict[int, Callable[[int], int]] | None = None,
        dropped_actor_data_keys: Set[int] | None = None,
    ) -> None:
        self._sound_remap = sound_remap
        self._int_remappers = actor_data_int_remappers or {}
        self._dropped_keys = dropped_actor_data_keys or set()

    def _remap_actor_data(self, wrapper: PacketWrapper) -> None:
        """Read ActorData list, filter/remap entries, write back."""
        entries = wrapper.read(ACTOR_DATA_LIST)
        if self._dropped_keys:
            entries = [e for e in entries if e.key not in self._dropped_keys]
        for entry in entries:
            if entry.key in self._int_remappers and entry.type_id in (DataItemType.INT, DataItemType.INT64):
                entry.value = self._int_remappers[entry.key](entry.value)
        wrapper.write(ACTOR_DATA_LIST, entries)

    def rewrite_level_sound_event(self, wrapper: PacketWrapper) -> None:
        """LevelSoundEventPacket (123): remap Event ID.

        Args:
            wrapper: Packet wrapper for LevelSoundEventPacket.
        """
        event_id = wrapper.read(UVAR_INT)  # Event ID
        wrapper.write(UVAR_INT, self._sound_remap(event_id))
        wrapper.passthrough_all()

    def rewrite_set_actor_data(self, wrapper: PacketWrapper) -> None:
        """SetActorDataPacket (39): remap ActorData sound fields.

        Args:
            wrapper: Packet wrapper for SetActorDataPacket.
        """
        wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
        self._remap_actor_data(wrapper)
        wrapper.passthrough_all()  # Synched Properties, Tick

    def rewrite_add_actor(self, wrapper: PacketWrapper) -> None:
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
        self._remap_actor_data(wrapper)
        wrapper.passthrough_all()  # Synched Properties, Actor Links

    def rewrite_add_item_actor(self, wrapper: PacketWrapper) -> None:
        """AddItemActorPacket (15): item entity spawn with ActorData.

        Args:
            wrapper: Packet wrapper for AddItemActorPacket.
        """
        wrapper.passthrough(VAR_INT64)  # Target Actor ID
        wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
        wrapper.passthrough(ITEM_INSTANCE)  # Item
        wrapper.passthrough(VEC3)  # Position
        wrapper.passthrough(VEC3)  # Velocity
        self._remap_actor_data(wrapper)
        wrapper.passthrough_all()  # From Fishing?

    def rewrite_add_player(self, wrapper: PacketWrapper) -> None:
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
        self._remap_actor_data(wrapper)
        wrapper.passthrough_all()  # Synched Properties, AbilitiesData, Actor Links

    def register(self, protocol: Protocol) -> None:
        """Register all clientbound sound/ActorData handlers on the protocol.

        Args:
            protocol: Protocol to register handlers on.
        """
        protocol.register_clientbound(PacketId.LEVEL_SOUND_EVENT, self.rewrite_level_sound_event)
        protocol.register_clientbound(PacketId.SET_ACTOR_DATA, self.rewrite_set_actor_data)
        protocol.register_clientbound(PacketId.ADD_ACTOR, self.rewrite_add_actor)
        protocol.register_clientbound(PacketId.ADD_ITEM_ACTOR, self.rewrite_add_item_actor)
        protocol.register_clientbound(PacketId.ADD_PLAYER, self.rewrite_add_player)
