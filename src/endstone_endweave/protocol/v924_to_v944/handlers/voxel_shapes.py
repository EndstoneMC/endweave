"""Handler for VoxelShapesPacket (337) -- v924 server to v944 client.

v944 added a `Custom Shape Count` (uint16) field at the end of the packet.
Append 0 so the v944 client can deserialize the v924 payload.
"""

from endstone_endweave.codec import USHORT_LE, PacketWrapper


def rewrite_voxel_shapes(wrapper: PacketWrapper) -> None:
    """Append the missing Custom Shape Count field for v944 clients.

    Args:
        wrapper: Packet wrapper for VoxelShapesPacket.
    """
    wrapper.passthrough_all()
    wrapper.write(USHORT_LE, 0)  # Custom Shape Count (new in v944)
