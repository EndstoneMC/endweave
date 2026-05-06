"""ClientMovementPredictionSync (322) -- v975 client to v944 server.

v975 added three trailing float32 movement attributes between Hunger and
EntityUniqueID. Strip them from the v975 client's packet so the v944 server
parses the remaining fields at the correct offsets.
"""

from endstone_endweave.codec import BOOL, BYTE, FLOAT_LE, VAR_INT64, PacketWrapper


def _passthrough_bitset(wrapper: PacketWrapper) -> None:
    while True:
        b = wrapper.passthrough(BYTE)
        if (b & 0x80) == 0:
            return


def rewrite_client_movement_prediction_sync(wrapper: PacketWrapper) -> None:
    _passthrough_bitset(wrapper)  # ActorDataFlagComponent
    for _ in range(9):  # ActorDataBoundingBoxComponent (3) + base movement attrs (6)
        wrapper.passthrough(FLOAT_LE)
    for _ in range(3):  # v975-only attributes
        wrapper.read(FLOAT_LE)
    wrapper.passthrough(VAR_INT64)  # Actor Unique ID
    wrapper.passthrough(BOOL)  # Flying
