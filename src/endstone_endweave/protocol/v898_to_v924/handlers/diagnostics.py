"""ServerboundDiagnostics handler for v898 to v924.

v924 appended a Memory Category Values list to the end of the packet.
Strip the trailing list so the v898 server only sees the 9 floats it expects.
"""

from endstone_endweave.codec import FLOAT_LE, REMAINING_BYTES, PacketWrapper


def rewrite_diagnostics(wrapper: PacketWrapper) -> None:
    """Strip v924 Memory Category Values from ServerboundDiagnostics.

    Args:
        wrapper: Packet wrapper for ServerboundDiagnostics.
    """
    for _ in range(9):
        wrapper.passthrough(FLOAT_LE)  # AvgFps .. AvgUnaccountedTimePercent
    wrapper.read(REMAINING_BYTES)  # discard Memory Category Values
