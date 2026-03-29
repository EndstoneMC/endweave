"""Handler for VoxelShapesPacket (337) -- v944 server to v924 client."""

from endstone_endweave.codec import REMAINING_BYTES, PacketWrapper


def rewrite_voxel_shapes(wrapper: PacketWrapper) -> None:
    """Strip the trailing Custom Shape Count field added in v944.

    Args:
        wrapper: Packet wrapper for VoxelShapesPacket.
    """
    # Custom Shape Count is the LAST field; passthrough everything, then strip trailing 2 bytes
    data = wrapper.read(REMAINING_BYTES)
    wrapper.write(REMAINING_BYTES, data[:-2])
