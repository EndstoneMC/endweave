"""Handler for EventPacket -- v860 server to v898 client."""

from endstone_endweave.codec import (
    BOOL,
    UVAR_INT,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
)


def rewrite_event(wrapper: PacketWrapper) -> None:
    """Insert the v898 event header fields.

    Args:
        wrapper: Packet wrapper for Event.
    """
    wrapper.passthrough(VAR_INT64)  # Target Actor ID
    event_type = wrapper.passthrough(VAR_INT)  # Event Type
    wrapper.passthrough(BOOL)  # Use Player ID
    wrapper.write(UVAR_INT, event_type)  # Event Data (new in v898)
    wrapper.passthrough_all()
