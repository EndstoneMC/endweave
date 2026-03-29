"""Event packet handler for v898 server to v860 client translation."""

from endstone_endweave.codec import (
    BOOL,
    UVAR_INT,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
)


def rewrite_event(wrapper: PacketWrapper) -> None:
    """Drop the extra v898 event header fields.

    Args:
        wrapper: Packet wrapper for Event.
    """
    wrapper.passthrough(VAR_INT64)  # Target Actor ID
    event_type = wrapper.passthrough(VAR_INT)  # Event Type
    wrapper.read(BOOL)  # Use Player ID
    duplicate_type = wrapper.read(UVAR_INT)  # Event Data
    if duplicate_type != event_type:
        raise ValueError(f"Mismatched event type header: {event_type} != {duplicate_type}")
    wrapper.passthrough_all()
