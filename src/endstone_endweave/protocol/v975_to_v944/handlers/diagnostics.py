"""ServerboundDiagnosticsPacket (315) -- v944 client to v975 server.

v975 added Entity Diagnostics and System Diagnostics arrays at the end;
append them as empty so the v975 server gets the fields it expects.
"""

from ....codec import UVAR_INT, PacketWrapper


def rewrite_diagnostics(wrapper: PacketWrapper) -> None:
    """Append empty Entity Diagnostics and System Diagnostics lists.

    Args:
        wrapper: Packet wrapper for ServerboundDiagnosticsPacket.
    """
    wrapper.passthrough_all()  # 9 floats + Memory Category Values
    wrapper.write(UVAR_INT, 0)  # Entity Diagnostics (empty)
    wrapper.write(UVAR_INT, 0)  # System Diagnostics (empty)
