"""ServerboundDiagnosticsPacket (315) -- v975 client to v944 server.

v975 appended two arrays (Entity Diagnostics, System Diagnostics) at the end.
Strip them so the v944 server only sees the fields it expects.
"""

from ....codec import BYTE, FLOAT_LE, INT64_LE, REMAINING_BYTES, UVAR_INT, PacketWrapper


def rewrite_diagnostics(wrapper: PacketWrapper) -> None:
    """Strip v975 Entity Diagnostics and System Diagnostics.

    Args:
        wrapper: Packet wrapper for ServerboundDiagnosticsPacket.
    """
    for _ in range(9):
        wrapper.passthrough(FLOAT_LE)  # AvgFps .. AvgUnaccountedTimePercent
    mem_count = wrapper.passthrough(UVAR_INT)  # Memory Category Values
    for _ in range(mem_count):
        wrapper.passthrough(BYTE)  # Category
        wrapper.passthrough(INT64_LE)  # Current Bytes
    wrapper.read(REMAINING_BYTES)  # discard Entity Diagnostics + System Diagnostics
