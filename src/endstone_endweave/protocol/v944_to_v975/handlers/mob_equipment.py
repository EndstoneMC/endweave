"""MobEquipmentPacket (31) -- v944 server to v975 client.

v944 wire wrote Slot, Selected Slot, Container ID as raw bytes (BDS uses
private byte members and _convertToBytes/_convertFromBytes machinery).
v975 widens them to uvarint32. Map each byte to a uvarint32 in clientbound,
and uvarint32 back to byte in serverbound.
"""

from ....codec import BYTE, ITEM_INSTANCE, UVAR_INT, UVAR_INT64, PacketWrapper


def rewrite_mob_equipment_clientbound(wrapper: PacketWrapper) -> None:
    """Map v944 byte slot fields to v975 uvarint32 (server to client).

    Args:
        wrapper: Packet wrapper for MobEquipmentPacket.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(ITEM_INSTANCE)  # Item
    wrapper.map(BYTE, UVAR_INT)  # Slot
    wrapper.map(BYTE, UVAR_INT)  # Selected Slot
    wrapper.map(BYTE, UVAR_INT)  # Container ID


def rewrite_mob_equipment_serverbound(wrapper: PacketWrapper) -> None:
    """Map v975 uvarint32 slot fields back to v944 byte (client to server).

    Args:
        wrapper: Packet wrapper for MobEquipmentPacket.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(ITEM_INSTANCE)  # Item
    wrapper.map(UVAR_INT, BYTE)  # Slot
    wrapper.map(UVAR_INT, BYTE)  # Selected Slot
    wrapper.map(UVAR_INT, BYTE)  # Container ID
