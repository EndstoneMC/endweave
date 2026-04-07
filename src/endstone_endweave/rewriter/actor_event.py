"""Reusable ActorEvent ID remapping for protocol translation.

Handles the ActorEventPacket (28) which carries a BYTE event ID that
shifts when new events are inserted between versions.  Upgrade shifts
IDs up; downgrade collapses the inserted range (cancelling packets
whose event ID falls in the new range the older version cannot handle).

See Also:
    com.viaversion.viaversion.rewriter.EntityRewriter
"""

from ..codec import BYTE, UVAR_INT64, PacketWrapper
from ..protocol import Protocol
from ..protocol.mapping_data import IdShift
from ..protocol.packet_ids import PacketId


class ActorEventRewriter:
    """Registers a clientbound handler for ActorEvent ID remapping.

    Args:
        shift: The ID shift describing new events inserted between versions.
        upgrade: True to shift IDs up (older server -> newer client),
            False to shift down and cancel unknown IDs (newer server -> older client).

    See Also:
        com.viaversion.viaversion.rewriter.EntityRewriter
    """

    def __init__(self, shift: IdShift, *, upgrade: bool) -> None:
        self._shift = shift
        self._upgrade = upgrade

    def _rewrite_upgrade(self, wrapper: PacketWrapper) -> None:
        wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
        event_id = wrapper.read(BYTE)  # Event ID
        wrapper.write(BYTE, self._shift.shift_up(event_id))  # Event ID

    def _rewrite_downgrade(self, wrapper: PacketWrapper) -> None:
        wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
        event_id = wrapper.read(BYTE)  # Event ID
        if self._shift.insert_at <= event_id < self._shift.insert_at + self._shift.count:
            wrapper.cancel()
            return
        wrapper.write(BYTE, self._shift.shift_down(event_id))  # Event ID

    def register(self, protocol: Protocol) -> None:
        """Register the clientbound ActorEvent handler on the protocol.

        Args:
            protocol: Protocol to register on.
        """
        handler = self._rewrite_upgrade if self._upgrade else self._rewrite_downgrade
        protocol.register_clientbound(PacketId.ACTOR_EVENT, handler)
