from endstone_endweave.codec import (
    BOOL,
    BYTE,
    ITEM_INSTANCE,
    ITEM_INSTANCE_V975,
    UINT_LE,
    UVAR_INT,
    UVAR_INT64,
    PacketWrapper,
)


def rewrite_mob_equipment_clientbound(wrapper: PacketWrapper) -> None:
    """Map v944 byte slot fields to v975 uvarint32 (server to client).

    Args:
        wrapper: Packet wrapper for MobEquipmentPacket.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.map(ITEM_INSTANCE, ITEM_INSTANCE_V975)  # Item
    wrapper.map(BYTE, UVAR_INT)  # Slot
    wrapper.map(BYTE, UVAR_INT)  # Selected Slot
    wrapper.map(BYTE, UVAR_INT)  # Container ID


def rewrite_mob_equipment_serverbound(wrapper: PacketWrapper) -> None:
    """Map v975 uvarint32 slot fields back to v944 byte (client to server).

    Args:
        wrapper: Packet wrapper for MobEquipmentPacket.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.map(ITEM_INSTANCE_V975, ITEM_INSTANCE)  # Item
    wrapper.map(UVAR_INT, BYTE)  # Slot
    wrapper.map(UVAR_INT, BYTE)  # Selected Slot
    wrapper.map(UVAR_INT, BYTE)  # Container ID


def rewrite_inventory_slot(wrapper: PacketWrapper) -> None:
    """InventorySlotPacket (50): rewrite v944 layout into v975 layout."""
    wrapper.passthrough(UVAR_INT)  # Container Id
    wrapper.passthrough(UVAR_INT)  # Slot

    # v944 always sends FullContainerName flat; v975 wraps it in optional bools.
    container_name = wrapper.read(BYTE)
    dynamic_id = wrapper.read(UVAR_INT)
    wrapper.write(BOOL, True)  # has Full Container Name
    wrapper.write(BYTE, container_name)
    wrapper.write(BOOL, True)  # has Dynamic ID
    wrapper.write(UINT_LE, dynamic_id)

    # v944 always sends Storage Item (with air shortcut); v975 makes it optional.
    storage = wrapper.read(ITEM_INSTANCE)
    if storage.network_id == 0:
        wrapper.write(BOOL, False)
    else:
        wrapper.write(BOOL, True)
        wrapper.write(ITEM_INSTANCE_V975, storage)

    # Item is always present in both versions.
    wrapper.map(ITEM_INSTANCE, ITEM_INSTANCE_V975)
