"""Item-stack rewriters: v944 -> v975 InventorySlotPacket.

v944 layout (verified by hex-dumping a live packet, e.g. 780000000000)::

    uvarint  Container Id
    uvarint  Slot
    uint8    FullContainerName.ContainerName    -- always present, no bool prefix
    uvarint  FullContainerName.DynamicID        -- always present, no bool prefix
    ItemInstance  Storage Item                  -- always present, air shortcut
    ItemInstance  Item                          -- always present, air shortcut

v975 layout (per protocol-docs r26_u2)::

    uvarint  Container Id
    uvarint  Slot
    bool+FullContainerName?                     -- optional
        uint8 ContainerName
        bool+uint32_le DynamicID                -- optional within FCN
    bool+cerealizer<...>::SerializedData?       -- optional Storage Item
    cerealizer<...>::SerializedData             -- Item (no air shortcut, all 6 fields)
"""

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
