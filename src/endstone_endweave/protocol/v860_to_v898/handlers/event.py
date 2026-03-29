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
    event_type = wrapper.read(VAR_INT)  # Event Type
    wrapper.write(VAR_INT, event_type)  # Event Type
    wrapper.write(BOOL, False)  # Use Player ID
    wrapper.write(UVAR_INT, event_type)  # Event Data
    wrapper.passthrough_all()
