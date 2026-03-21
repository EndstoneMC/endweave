"""Handlers for Data-Driven UI packets (333, 334) -- v924 to v944.

ClientBoundDataDrivenUIShowScreen (333):
    v944 added uint32 FormID and uint32 DataInstanceID after ScreenID.

ClientBoundDataDrivenUICloseAllScreens (334):
    v924 sends CloseAllScreens with no fields. v944 renamed it to CloseScreen
    and added a uint32 FormID field.
"""

from endstone_endweave.codec import UINT_LE, PacketWrapper


def rewrite_show_screen(wrapper: PacketWrapper) -> None:
    """Append the missing FormID and DataInstanceID for v944 clients.

    Args:
        wrapper: Packet wrapper for ShowScreen.
    """
    wrapper.passthrough_all()
    wrapper.write(UINT_LE, 0)  # FormId (new in v944)
    wrapper.write(UINT_LE, 0)  # DataInstanceId (new in v944)


def rewrite_close_all_screens(wrapper: PacketWrapper) -> None:
    """Append the missing FormID for v944 clients.

    Args:
        wrapper: Packet wrapper for CloseAllScreens / CloseScreen.
    """
    wrapper.passthrough_all()
    wrapper.write(UINT_LE, 0)  # FormId (new in v944, 0 = close all)
