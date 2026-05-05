"""Item-stack rewriters: v975 -> v944 InventorySlotPacket. Reverse of v944_to_v975."""

from ....codec import (
    BOOL,
    BYTE,
    ITEM_INSTANCE,
    ITEM_INSTANCE_V975,
    UINT_LE,
    UVAR_INT,
    PacketWrapper,
)


def rewrite_inventory_slot(wrapper: PacketWrapper) -> None:
    """InventorySlotPacket (50): rewrite v975 layout into v944 layout."""
    wrapper.passthrough(UVAR_INT)  # Container Id
    wrapper.passthrough(UVAR_INT)  # Slot

    # v975 has optional FullContainerName; v944 always has it.
    container_name = 0
    dynamic_id = 0
    if wrapper.read(BOOL):  # FCN present?
        container_name = wrapper.read(BYTE)
        if wrapper.read(BOOL):  # DynID present?
            dynamic_id = wrapper.read(UINT_LE)
    wrapper.write(BYTE, container_name)
    wrapper.write(UVAR_INT, dynamic_id)

    # v975 has optional Storage Item; v944 always has it (with air shortcut).
    if wrapper.read(BOOL):  # Storage present?
        storage = wrapper.read(ITEM_INSTANCE_V975)
    else:
        from ....codec.types.item import ItemInstance

        storage = ItemInstance(network_id=0)
    wrapper.write(ITEM_INSTANCE, storage)

    # Item is always present in both versions.
    item = wrapper.read(ITEM_INSTANCE_V975)
    wrapper.write(ITEM_INSTANCE, item)
