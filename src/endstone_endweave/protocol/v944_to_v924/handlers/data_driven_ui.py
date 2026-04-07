"""Handlers for Data-Driven UI packets (333, 334) -- v944 to v924."""

from ....codec import INT_LE, STRING, OptionalType, PacketWrapper


def rewrite_show_screen(wrapper: PacketWrapper) -> None:
    """Strip the v944-only ShowScreen fields.

    Args:
        wrapper: Packet wrapper for ShowScreen.
    """
    wrapper.passthrough(STRING)  # ScreenId
    wrapper.read(INT_LE)  # FormId
    wrapper.read(OptionalType(INT_LE))  # DataInstanceId


def rewrite_close_screen(wrapper: PacketWrapper) -> None:
    """Strip the v944 optional FormID from CloseScreen.

    Args:
        wrapper: Packet wrapper for CloseScreen.
    """
    wrapper.read(OptionalType(INT_LE))  # FormId
