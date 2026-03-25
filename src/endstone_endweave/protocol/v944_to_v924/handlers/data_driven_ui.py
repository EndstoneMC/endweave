"""Handlers for Data-Driven UI packets (333, 334) -- v944 to v924."""

from endstone_endweave.codec import BOOL, INT_LE, STRING, PacketWrapper


def rewrite_show_screen(wrapper: PacketWrapper) -> None:
    """Strip the v944-only ShowScreen fields.

    Args:
        wrapper: Packet wrapper for ShowScreen.
    """
    wrapper.passthrough(STRING)
    wrapper.read(INT_LE)
    if wrapper.read(BOOL):
        wrapper.read(INT_LE)


def rewrite_close_screen(wrapper: PacketWrapper) -> None:
    """Strip the v944 optional FormID from CloseScreen.

    Args:
        wrapper: Packet wrapper for CloseScreen.
    """
    if wrapper.read(BOOL):
        wrapper.read(INT_LE)
