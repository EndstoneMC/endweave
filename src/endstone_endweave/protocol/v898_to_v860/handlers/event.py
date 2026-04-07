"""Event packet handler for v898 server to v860 client translation."""

from ....codec import (
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
    wrapper.passthrough(VAR_INT)  # Event Type
    wrapper.passthrough(BOOL)  # Use Player ID
    wrapper.read(UVAR_INT)  # Event Data (strip for v860)
    wrapper.passthrough_all()
