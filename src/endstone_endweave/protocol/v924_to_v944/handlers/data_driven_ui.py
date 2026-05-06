"""Handlers for Data-Driven UI packets (333, 334) -- v924 to v944."""

from endstone_endweave.codec import INT_LE, OptionalType, PacketWrapper


def rewrite_show_screen(wrapper: PacketWrapper) -> None:
    """Append the missing v944 fields for ShowScreen.

    Args:
        wrapper: Packet wrapper for ShowScreen.
    """
    wrapper.passthrough_all()  # ScreenId
    wrapper.write(INT_LE, 0)  # FormId
    wrapper.write(OptionalType(INT_LE), None)  # DataInstanceId


def rewrite_close_all_screens(wrapper: PacketWrapper) -> None:
    """Rewrite v924 CloseAllScreens into the v944 CloseScreen form.

    Args:
        wrapper: Packet wrapper for CloseAllScreens / CloseScreen.
    """
    wrapper.passthrough_all()
    wrapper.write(OptionalType(INT_LE), None)  # FormId
