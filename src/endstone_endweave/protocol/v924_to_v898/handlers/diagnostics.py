"""ServerboundDiagnostics handler for v924 to v898.

v924 expects a Memory Category Values list after the 9 diagnostic floats.
Append an empty list so the v924 server gets the field it expects.
"""

from endstone_endweave.codec import FLOAT_LE, UVAR_INT, PacketWrapper


def rewrite_diagnostics(wrapper: PacketWrapper) -> None:
    """Append empty Memory Category Values to ServerboundDiagnostics.

    Args:
        wrapper: Packet wrapper for ServerboundDiagnostics.
    """
    for _ in range(9):
        wrapper.passthrough(FLOAT_LE)  # AvgFps .. AvgUnaccountedTimePercent
    wrapper.write(UVAR_INT, 0)  # Memory Category Values (empty)
