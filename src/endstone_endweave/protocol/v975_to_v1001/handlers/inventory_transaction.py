"""Serverbound InventoryTransaction(30) handler for the v975 <- v1001 delta.

Direction
---------
InventoryTransactionPacket is SERVERBOUND (client -> server). In a v1001 client's
translation chain ``[base(944), v944_to_v975, v975_to_v1001]`` the serverbound
handlers run NEW-to-OLD (newest layer first): this handler READS the v1001 (NEW)
wire and WRITES the v975 (OLD) wire. The v944_to_v975 layer registers NO
InventoryTransaction handler because the packet is byte-identical at v944 and
v975 (it is "OLD" at both), so the v975 output produced here passes straight
through to the v944 server unchanged. This handler therefore only needs to
convert v1001 (NEW) -> v975 (OLD).

Why the OLD codec types cannot be reused for READING
----------------------------------------------------
v1001 restructured the packet in three places that the existing OLD
``INVENTORY_ACTION`` / ``ITEM_INSTANCE`` codecs cannot decode:

  * Three new presence bools at the top level: ``hasLegacy`` (a real gate),
    ``hasType`` (always true), ``hasActions`` (always true).
  * The InventoryAction source encoding was rewritten from a switch on
    ``SourceType`` into nested presence bools (present/hasContainerID, then
    present/hasFlags), and the source WindowID narrowed from a zigzag varint32
    to a SIGNED int8.
  * The embedded item descriptors flipped from OLD ``ITEM_INSTANCE`` (varint32
    id + 1-byte air shortcut) to NEW ``ITEM_INSTANCE_V975`` (int16 id, no air
    shortcut, HasNetIdVariant) for InventoryAction.OldItem/NewItem and for
    UseItem/UseItemOnEntity/ReleaseItem HeldItem.

So the action source and all item fields are hand-read in the NEW shape, then
re-emitted in the OLD shape. The OLD action *write* is delegated to the existing
``INVENTORY_ACTION`` codec by rebuilding an ``InventoryAction`` dataclass -- its
writer already emits the OLD switch + varint WindowID + OLD items, byte-for-byte.

Cross-checked against gophertunnel v1.57.0 (protocol 1001, commit ~6353c49 / #438)
and CloudburstMC/Protocol; the two agree byte-for-byte. The action-source WindowID
is confirmed SIGNED int8 at v1001 (gophertunnel ``r.Int8`` / Netty signed
``readByte``); sign-extension is load-bearing because container IDs can legitimately
be negative (e.g. -1 / 0xFF for "no container") and a naive unsigned read would emit
255 instead of -1 in the OLD VAR_INT to the v944 server.

This packet is forwarded to the server -- it is NOT cancelled.
"""

from endstone_endweave.codec import (
    BLOCK_POS,
    BOOL,
    BYTE,
    INT8,
    INVENTORY_ACTION,
    ITEM_INSTANCE,
    ITEM_INSTANCE_V975,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VEC3,
    ComplexInventoryTransactionType,
    InventoryAction,
    InventorySourceType,
    PacketWrapper,
)


def _rewrite_action(wrapper: PacketWrapper) -> None:
    """Translate one InventoryAction NEW (v1001) -> OLD (v975).

    NEW source encoding (read): SourceType, then two presence-bool pairs --
    [present, hasContainerID] gating a SIGNED int8 WindowID, and
    [present, hasFlags] gating a uvarint32 SourceFlags -- followed by the slot
    and the two NEW item descriptors.

    OLD source encoding (written via ``INVENTORY_ACTION``): SourceType, then a
    switch -- VAR_INT WindowID for Container/NonImplemented, uvarint32 SourceFlags
    for WorldInteraction, nothing otherwise -- then the slot and the two OLD items.

    Args:
        wrapper: Packet wrapper positioned at the start of an InventoryAction.
    """
    source_type = wrapper.read(UVAR_INT)  # SourceType (encoding unchanged)

    # NEW: [present, hasContainerID] presence-bool pair gating the WindowID.
    wrapper.read(BOOL)  # present sentinel (NEW) -- discard
    has_container_id = wrapper.read(BOOL)  # hasContainerID (NEW)
    window_id: int | None = None
    if has_container_id:
        window_id = wrapper.read(INT8)  # WindowID: SIGNED int8 (NEW) -- sign-extended

    # NEW: [present, hasFlags] presence-bool pair gating the SourceFlags.
    wrapper.read(BOOL)  # present sentinel (NEW) -- discard
    has_flags = wrapper.read(BOOL)  # hasFlags (NEW)
    source_flags: int | None = None
    if has_flags:
        source_flags = wrapper.read(UVAR_INT)  # SourceFlags (width unchanged)

    slot = wrapper.read(UVAR_INT)  # InventorySlot (unchanged)
    old_item = wrapper.read(ITEM_INSTANCE_V975)  # OldItem (NEW descriptor)
    new_item = wrapper.read(ITEM_INSTANCE_V975)  # NewItem (NEW descriptor)

    # OLD write: re-derive the switch form from SourceType. The OLD writer emits
    # VAR_INT WindowID for Container/NonImplemented and uvarint32 SourceFlags for
    # WorldInteraction, exactly mirroring the values just read. ITEM_INSTANCE.write
    # re-emits the OLD descriptor (varint32 id + 1-byte air shortcut) from the
    # shared ItemInstance dataclass.
    wrapper.write(
        INVENTORY_ACTION,
        InventoryAction(
            source_type=source_type,
            window_id=window_id,
            source_flags=source_flags,
            slot=slot,
            old_item=old_item,
            new_item=new_item,
        ),
    )


def rewrite_inventory_transaction(wrapper: PacketWrapper) -> None:
    """InventoryTransaction (30): convert the v1001 (NEW) wire to the v975 (OLD) wire.

    Reads the NEW packet a 1.26.30 client sends and writes the OLD packet the
    v944/v975 server expects: drops the three v1001 presence bools, rebuilds the
    OLD InventoryAction source switch (with a sign-extended int8 WindowID), and
    narrows every changed scalar back to its OLD width. Items flip from the NEW
    descriptor back to the OLD descriptor.

    Args:
        wrapper: Packet wrapper for InventoryTransactionPacket.
    """
    wrapper.passthrough(VAR_INT)  # LegacyRequestID (unchanged at both versions)

    # NEW: hasLegacy presence bool -- gates the LegacySetItemSlots block. The OLD
    # wire has no such bool (it gated slots on LegacyRequestID != 0), so consume
    # it and do NOT write it. When true, LegacyRequestID < -1 hence != 0, so the
    # OLD decoder will look for the slots we are about to write -- they match.
    has_legacy = wrapper.read(BOOL)  # hasLegacy (NEW) -- discard
    if has_legacy:
        # LegacySetItemSlots: element layout is IDENTICAL at NEW and OLD, so copy
        # it through verbatim (no presence bools inside the elements).
        slot_count = wrapper.passthrough(UVAR_INT)
        for _ in range(slot_count):
            wrapper.passthrough(BYTE)  # ContainerID / Container Enum (raw byte)
            slots_len = wrapper.passthrough(UVAR_INT)
            for _ in range(slots_len):
                wrapper.passthrough(BYTE)  # Slot

    wrapper.read(BOOL)  # hasType (NEW, always true) -- discard

    transaction_type = wrapper.passthrough(UVAR_INT)  # TransactionType tag (unchanged)

    wrapper.read(BOOL)  # hasActions (NEW, always true) -- discard

    # InventoryActions: count unchanged; each action is restructured NEW -> OLD.
    action_count = wrapper.passthrough(UVAR_INT)
    for _ in range(action_count):
        _rewrite_action(wrapper)

    # Trailing TransactionData, switch on TransactionType.
    if transaction_type == ComplexInventoryTransactionType.ITEM_USE_TRANSACTION:
        _rewrite_use_item(wrapper)
    elif transaction_type == ComplexInventoryTransactionType.ITEM_USE_ON_ENTITY_TRANSACTION:
        _rewrite_use_item_on_entity(wrapper)
    elif transaction_type == ComplexInventoryTransactionType.ITEM_RELEASE_TRANSACTION:
        _rewrite_release_item(wrapper)
    # NORMAL_TRANSACTION(0) and INVENTORY_MISMATCH(1): no trailing data either side.
    # A well-formed packet has no further bytes; to_bytes() appends none.


def _rewrite_use_item(wrapper: PacketWrapper) -> None:
    """UseItemTransactionData (type 2): NEW widths -> OLD widths.

    Args:
        wrapper: Packet wrapper positioned at the UseItem trailing data.
    """
    action_type = wrapper.read(VAR_INT)  # ActionType: signed varint32 (NEW)
    wrapper.write(UVAR_INT, action_type)  # -> uvarint32 (OLD); small non-negative enum

    trigger_type = wrapper.read(BYTE)  # TriggerType: uint8 (NEW)
    wrapper.write(UVAR_INT, trigger_type)  # -> uvarint32 (OLD)

    wrapper.passthrough(BLOCK_POS)  # BlockPosition (unchanged: 3 zigzag varint32)

    block_face = wrapper.read(INT8)  # BlockFace: uint8 on the wire (NEW), sign-extended
    wrapper.write(VAR_INT, block_face)  # -> signed varint32 (OLD); -1 face round-trips

    wrapper.passthrough(VAR_INT)  # HotBarSlot (unchanged: signed varint32)
    wrapper.map(ITEM_INSTANCE_V975, ITEM_INSTANCE)  # HeldItem (NEW -> OLD descriptor)
    wrapper.passthrough(VEC3)  # Position (unchanged: 3x float32 LE)
    wrapper.passthrough(VEC3)  # ClickedPosition (unchanged)
    wrapper.passthrough(UVAR_INT)  # BlockRuntimeID (unchanged: uvarint32)

    client_prediction = wrapper.read(BYTE)  # ClientPrediction: uint8 (NEW)
    wrapper.write(UVAR_INT, client_prediction)  # -> uvarint32 (OLD)

    wrapper.passthrough(BYTE)  # ClientCooldownState (unchanged: 1 byte at v975 and v1001)


def _rewrite_use_item_on_entity(wrapper: PacketWrapper) -> None:
    """UseItemOnEntityTransactionData (type 3): NEW widths -> OLD widths.

    Args:
        wrapper: Packet wrapper positioned at the UseItemOnEntity trailing data.
    """
    wrapper.passthrough(UVAR_INT64)  # TargetEntityRuntimeID (unchanged: uvarint64)

    action_type = wrapper.read(VAR_INT)  # ActionType: signed varint32 (NEW)
    wrapper.write(UVAR_INT, action_type)  # -> uvarint32 (OLD); small non-negative enum

    wrapper.passthrough(VAR_INT)  # HotBarSlot (unchanged)
    wrapper.map(ITEM_INSTANCE_V975, ITEM_INSTANCE)  # HeldItem (NEW -> OLD descriptor)
    wrapper.passthrough(VEC3)  # Position (unchanged)
    wrapper.passthrough(VEC3)  # ClickedPosition (unchanged)


def _rewrite_release_item(wrapper: PacketWrapper) -> None:
    """ReleaseItemTransactionData (type 4): NEW widths -> OLD widths.

    Args:
        wrapper: Packet wrapper positioned at the ReleaseItem trailing data.
    """
    action_type = wrapper.read(VAR_INT)  # ActionType: signed varint32 (NEW)
    wrapper.write(UVAR_INT, action_type)  # -> uvarint32 (OLD); small non-negative enum

    wrapper.passthrough(VAR_INT)  # HotBarSlot (unchanged)
    wrapper.map(ITEM_INSTANCE_V975, ITEM_INSTANCE)  # HeldItem (NEW -> OLD descriptor)
    wrapper.passthrough(VEC3)  # HeadPosition (unchanged)


# ---------------------------------------------------------------------------
# CLIENTBOUND (server -> client): mirror image of the serverbound handler above.
# A v944 server emits the OLD (v975-shaped) InventoryTransaction; a 1.26.30 client
# expects the NEW (v1001) wire, so this reads OLD and writes NEW -- the exact
# inverse of every transform in the serverbound path. Confirmed via offline
# gophertunnel v1.57.0 validation: the untranslated clientbound packet fails with
# "expected presence bool to be true" (the client reads the actions count as the
# new hasType bool and desyncs); this restores the v1001 framing.
# ---------------------------------------------------------------------------


def _action_old_to_new(wrapper: PacketWrapper) -> None:
    """Translate one InventoryAction OLD (v975) -> NEW (v1001).

    Reads the OLD switch-form source (via INVENTORY_ACTION) and re-emits the NEW
    nested-presence-bool source (present/hasContainerID + signed int8 WindowID,
    present/hasFlags + uvarint Flags) with NEW item descriptors.
    """
    action = wrapper.read(INVENTORY_ACTION)  # OLD: source switch + ITEM_INSTANCE items

    wrapper.write(UVAR_INT, action.source_type)
    has_container = action.source_type in (
        InventorySourceType.CONTAINER_INVENTORY,
        InventorySourceType.NON_IMPLEMENTED_FEATURE_TODO,
    )
    has_flags = action.source_type == InventorySourceType.WORLD_INTERACTION
    # WindowID group: present sentinel + hasContainerID gate.
    wrapper.write(BOOL, True)
    wrapper.write(BOOL, has_container)
    if has_container:
        assert action.window_id is not None
        wrapper.write(INT8, action.window_id)  # narrow VAR_INT -> signed int8 (NEW)
    # Flags group: present sentinel + hasFlags gate.
    wrapper.write(BOOL, True)
    wrapper.write(BOOL, has_flags)
    if has_flags:
        assert action.source_flags is not None
        wrapper.write(UVAR_INT, action.source_flags)
    wrapper.write(UVAR_INT, action.slot)
    wrapper.write(ITEM_INSTANCE_V975, action.old_item)  # OLD -> NEW descriptor
    wrapper.write(ITEM_INSTANCE_V975, action.new_item)


def _use_item_old_to_new(wrapper: PacketWrapper) -> None:
    """UseItemTransactionData (type 2): OLD widths -> NEW widths."""
    action_type = wrapper.read(UVAR_INT)  # OLD uvarint32
    wrapper.write(VAR_INT, action_type)  # -> NEW signed varint32

    trigger_type = wrapper.read(UVAR_INT)  # OLD uvarint32
    wrapper.write(BYTE, trigger_type)  # -> NEW uint8

    wrapper.passthrough(BLOCK_POS)  # unchanged

    block_face = wrapper.read(VAR_INT)  # OLD signed varint32 (-1 = no face)
    wrapper.write(INT8, block_face)  # -> NEW uint8 (-1 -> 0xFF)

    wrapper.passthrough(VAR_INT)  # HotBarSlot (unchanged)
    wrapper.map(ITEM_INSTANCE, ITEM_INSTANCE_V975)  # HeldItem OLD -> NEW
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC3)  # ClickedPosition
    wrapper.passthrough(UVAR_INT)  # BlockRuntimeID

    client_prediction = wrapper.read(UVAR_INT)  # OLD uvarint32
    wrapper.write(BYTE, client_prediction)  # -> NEW uint8

    wrapper.passthrough(BYTE)  # ClientCooldownState (unchanged)


def _use_item_on_entity_old_to_new(wrapper: PacketWrapper) -> None:
    """UseItemOnEntityTransactionData (type 3): OLD widths -> NEW widths."""
    wrapper.passthrough(UVAR_INT64)  # TargetEntityRuntimeID (unchanged)
    action_type = wrapper.read(UVAR_INT)
    wrapper.write(VAR_INT, action_type)
    wrapper.passthrough(VAR_INT)  # HotBarSlot
    wrapper.map(ITEM_INSTANCE, ITEM_INSTANCE_V975)  # HeldItem OLD -> NEW
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC3)  # ClickedPosition


def _release_item_old_to_new(wrapper: PacketWrapper) -> None:
    """ReleaseItemTransactionData (type 4): OLD widths -> NEW widths."""
    action_type = wrapper.read(UVAR_INT)
    wrapper.write(VAR_INT, action_type)
    wrapper.passthrough(VAR_INT)  # HotBarSlot
    wrapper.map(ITEM_INSTANCE, ITEM_INSTANCE_V975)  # HeldItem OLD -> NEW
    wrapper.passthrough(VEC3)  # HeadPosition


def rewrite_inventory_transaction_clientbound(wrapper: PacketWrapper) -> None:
    """InventoryTransaction (30) CLIENTBOUND: convert the v975 (OLD) wire to v1001 (NEW).

    The v944/v975 server emits the OLD wire; a 1.26.30 client expects the NEW wire
    (three new top-level presence bools, nested presence bools + int8 WindowID in
    each action, and NEW item descriptors). Inserts the v1001 framing the OLD form
    lacks. Mirror of ``rewrite_inventory_transaction`` (the serverbound path).

    Args:
        wrapper: Packet wrapper for a clientbound InventoryTransactionPacket.
    """
    legacy_request_id = wrapper.passthrough(VAR_INT)  # unchanged

    # NEW hasLegacy bool: present iff the OLD form carried LegacySetItemSlots, which
    # it gates on LegacyRequestID != 0. Write the bool, then copy the slots verbatim
    # (element layout identical OLD/NEW). Clientbound packets normally use id 0.
    has_legacy = legacy_request_id != 0
    wrapper.write(BOOL, has_legacy)
    if has_legacy:
        slot_count = wrapper.passthrough(UVAR_INT)
        for _ in range(slot_count):
            wrapper.passthrough(BYTE)  # ContainerID
            slots_len = wrapper.passthrough(UVAR_INT)
            for _ in range(slots_len):
                wrapper.passthrough(BYTE)  # Slot

    wrapper.write(BOOL, True)  # hasType (NEW, always true)
    transaction_type = wrapper.passthrough(UVAR_INT)  # TransactionType tag (unchanged)
    wrapper.write(BOOL, True)  # hasActions (NEW, always true)

    action_count = wrapper.passthrough(UVAR_INT)
    for _ in range(action_count):
        _action_old_to_new(wrapper)

    if transaction_type == ComplexInventoryTransactionType.ITEM_USE_TRANSACTION:
        _use_item_old_to_new(wrapper)
    elif transaction_type == ComplexInventoryTransactionType.ITEM_USE_ON_ENTITY_TRANSACTION:
        _use_item_on_entity_old_to_new(wrapper)
    elif transaction_type == ComplexInventoryTransactionType.ITEM_RELEASE_TRANSACTION:
        _release_item_old_to_new(wrapper)
    # NORMAL(0)/MISMATCH(1): no trailing data either side.
