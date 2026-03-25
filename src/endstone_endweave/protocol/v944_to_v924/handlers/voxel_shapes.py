"""Handler for VoxelShapesPacket (337) -- v944 server to v924 client."""

from endstone_endweave.codec import USHORT_LE, PacketWrapper


def rewrite_voxel_shapes(wrapper: PacketWrapper) -> None:
    """Strip the Custom Shape Count field added in v944.

    Args:
        wrapper: Packet wrapper for VoxelShapesPacket.
    """
    wrapper.read(USHORT_LE)
    wrapper.passthrough_all()
