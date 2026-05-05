"""MobEquipmentPacket (31) -- v975 server to v944 client.

v975 wire writes Slot, Selected Slot, Container ID as uvarint32. v944 expects
raw bytes (BDS used _convertToBytes/_convertFromBytes around private byte
members). Map uvarint32 to byte in clientbound, byte back to uvarint32 in
serverbound.
"""

from ....codec import BYTE, ITEM_INSTANCE, UVAR_INT, UVAR_INT64, PacketWrapper


def rewrite_mob_equipment_clientbound(wrapper: PacketWrapper) -> None:
    """Map v975 uvarint32 slot fields to v944 byte (server to client).

    Args:
        wrapper: Packet wrapper for MobEquipmentPacket.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(ITEM_INSTANCE)  # Item
    wrapper.map(UVAR_INT, BYTE)  # Slot
    wrapper.map(UVAR_INT, BYTE)  # Selected Slot
    wrapper.map(UVAR_INT, BYTE)  # Container ID


def rewrite_mob_equipment_serverbound(wrapper: PacketWrapper) -> None:
    """Map v944 byte slot fields back to v975 uvarint32 (client to server).

    Args:
        wrapper: Packet wrapper for MobEquipmentPacket.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(ITEM_INSTANCE)  # Item
    wrapper.map(BYTE, UVAR_INT)  # Slot
    wrapper.map(BYTE, UVAR_INT)  # Selected Slot
    wrapper.map(BYTE, UVAR_INT)  # Container ID
