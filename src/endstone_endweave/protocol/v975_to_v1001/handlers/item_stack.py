"""Clientbound item-format handlers for the v975 -> v1001 delta (MC 1.26.20 -> 1.26.30).

Why these live in the v975->v1001 layer (NOT v944->v975)
--------------------------------------------------------
The Bedrock item DESCRIPTOR wire format did NOT change between protocol 975 and
1001: the v1001 codec (``ItemInstanceNew`` / cerealizer ``SerializedData``) is
byte-for-byte identical to the v975 one. What changed at 1.26.30 is *which
packets use which descriptor*.

Cross-checked against Sandertv/gophertunnel master (CurrentProtocol == 1001),
commit "Updated for 1.26.30" (#438):

  * InventorySlot(50) and MobEquipment(31) already used the NEW descriptor
    (``ItemInstanceNew``) at protocol 975 -- so the v944->v975 layer correctly
    upgrades their items there, and that is right for BOTH a v975-terminal
    client and a v1001-terminal client.
  * InventoryContent(49) and MobArmourEquipment(32) used the OLD descriptor
    (``ItemInstance``: varint32 NetworkID + 1-byte air shortcut) at protocol
    944 AND 975, and were flipped to the NEW descriptor (``ItemInstanceNew``:
    int16 NetworkID, no air shortcut) ONLY at protocol 1001.

That divergence is *target-version dependent*: a real v975 client (1.26.20)
still expects the OLD descriptor for packets 49/32, while a v1001 client
(1.26.30) expects the NEW one. Doing the upgrade in the v944->v975 layer would
therefore corrupt these two packets for a genuine v975 client. The conversion
must happen here, in the v975->v1001 delta, where it only affects v1001-bound
traffic. This matches the chain wiring in pipeline.py: a v975 client gets the
chain ``[v944_to_v975]`` and terminates at v975 (these handlers never run); a
v1001 client gets ``[v944_to_v975, v975_to_v1001]`` and these handlers upgrade
49/32 the rest of the way.

This is the spawn drop fix: a 1.26.30 client receives its full inventory
(InventoryContent) and worn armor (MobArmourEquipment) at spawn, encoded with the
OLD descriptor by the v944 server. Without this upgrade the client mis-parses the
items and the count-prefixed array underflows, so it desyncs and drops. The break
is NOT gated on having gear: OLD air is 1 byte (varint32 0 + shortcut) while NEW
air is a full ~8-byte record (verified on gophertunnel v1.57.0 ItemInstanceNew,
which has no air shortcut), so even an empty 36-slot inventory / 5-slot all-air
armor packet is byte-wider in NEW and underflows on the very first slot. The
symptom therefore affects all 1.26.30 clients, geared or not.

Air handling
------------
``ITEM_INSTANCE`` (OLD) returns early on ``network_id == 0`` (the 1-byte air
shortcut) and yields an all-zero ItemInstance. ``ITEM_INSTANCE_V975`` (NEW) has
NO air shortcut and re-emits air as a full record (int16 0, uint16 0, uvarint 0
aux, bool False, uvarint 0 runtime id, uvarint 0 user-data length). Routing the
value through ``wrapper.map(ITEM_INSTANCE, ITEM_INSTANCE_V975)`` performs exactly
that re-encoding via the shared ItemInstance dataclass.

Descriptors that must NOT be touched at v1001 (verified on gophertunnel master):
AddPlayer(12).HeldItem and AddItemActor(15).Item still use the OLD descriptor at
v1001, so the existing ``passthrough(ITEM_INSTANCE)`` for those in
rewriter/sound.py is already correct and is intentionally left unchanged.
"""

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

# MobArmourEquipment carries a fixed arity of 5 item fields with no count prefix:
# Helmet, Chestplate, Leggings, Boots, Body. The Body slot already existed at v975.
_ARMOUR_SLOT_COUNT = 5


def rewrite_inventory_content(wrapper: PacketWrapper) -> None:
    """InventoryContent (49): upgrade OLD item descriptors to the v1001 (NEW) format.

    v975 and v1001 share the same packet *structure*; only the two embedded item
    descriptors changed from OLD (``ITEM_INSTANCE``) to NEW (``ITEM_INSTANCE_V975``)
    at v1001.

    Wire layout (identical field order at v975 and v1001)::

        uvarint32              WindowID
        uvarint32 count        Content array
          N x item             (OLD @ v975 -> NEW @ v1001)
        FullContainerName      uint8 ContainerID + Optional<uint32 LE> DynamicID
        item                   StorageItem (OLD @ v975 -> NEW @ v1001)

    Args:
        wrapper: Packet wrapper for InventoryContentPacket.
    """
    wrapper.passthrough(UVAR_INT)  # WindowID

    # Content: count-prefixed item array. Convert each item OLD -> NEW. The count
    # itself is unchanged, so pass it through, then re-encode every element.
    content_count = wrapper.passthrough(UVAR_INT)
    for _ in range(content_count):
        wrapper.map(ITEM_INSTANCE, ITEM_INSTANCE_V975)

    # FullContainerName: uint8 ContainerID + Optional<uint32 LE> DynamicContainerID.
    # Byte-identical between v975 and v1001 -- copy verbatim.
    wrapper.passthrough(BYTE)  # ContainerID
    has_dynamic_id = wrapper.passthrough(BOOL)  # Optional<uint32 LE> present flag
    if has_dynamic_id:
        wrapper.passthrough(UINT_LE)  # DynamicContainerID

    # Trailing StorageItem (present since v975) -- convert OLD -> NEW.
    wrapper.map(ITEM_INSTANCE, ITEM_INSTANCE_V975)


def rewrite_mob_armour_equipment(wrapper: PacketWrapper) -> None:
    """MobArmourEquipment (32): upgrade the 5 OLD armor item descriptors to NEW.

    Wire layout (identical field order at v975 and v1001)::

        uvarint64   EntityRuntimeID
        item        Helmet      (OLD @ v975 -> NEW @ v1001)
        item        Chestplate  (OLD @ v975 -> NEW @ v1001)
        item        Leggings    (OLD @ v975 -> NEW @ v1001)
        item        Boots       (OLD @ v975 -> NEW @ v1001)
        item        Body        (OLD @ v975 -> NEW @ v1001)

    Five consecutive item fields, NO count prefix (fixed arity of 5). Air slots
    are re-emitted as full NEW air records, not the OLD 1-byte shortcut.

    Args:
        wrapper: Packet wrapper for MobArmourEquipmentPacket.
    """
    wrapper.passthrough(UVAR_INT64)  # EntityRuntimeID
    for _ in range(_ARMOUR_SLOT_COUNT):
        wrapper.map(ITEM_INSTANCE, ITEM_INSTANCE_V975)
