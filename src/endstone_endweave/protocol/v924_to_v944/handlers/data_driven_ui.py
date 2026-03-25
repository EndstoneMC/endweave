"""Handlers for Data-Driven UI packets (333, 334) -- v924 to v944."""

from endstone_endweave.codec import BOOL, INT_LE, PacketWrapper


def rewrite_show_screen(wrapper: PacketWrapper) -> None:
    """Append the missing v944 fields for ShowScreen.

    Args:
        wrapper: Packet wrapper for ShowScreen.
    """
    wrapper.passthrough_all()
    wrapper.write(INT_LE, 0)  # FormId
    wrapper.write(BOOL, False)  # DataInstanceId


def rewrite_close_all_screens(wrapper: PacketWrapper) -> None:
    """Rewrite v924 CloseScreen into the v944 optional FormID form.

    Args:
        wrapper: Packet wrapper for CloseAllScreens / CloseScreen.
    """
    wrapper.passthrough_all()
    wrapper.write(BOOL, False)  # FormId
