"""Inventory-related compound types (InventoryAction)."""

from dataclasses import dataclass

from ..reader import PacketReader
from ..writer import PacketWriter
from .enums import InventorySourceType
from .item import ITEM_INSTANCE, ItemInstance
from .primitives import UVAR_INT, VAR_INT, Type


@dataclass
class InventoryAction:
    """A single InventoryAction entry in InventoryTransactionPacket.

    Attributes:
        source_type: Source type ID (0=Container, 1=Global, 2=World, 3=Creative).
        window_id: Window ID (only for Container/NonImplementedFeature source types).
        source_flags: Source flags (only for WorldInteraction source type).
        slot: Inventory slot index.
        old_item: Item before the action.
        new_item: Item after the action.
    """

    source_type: int
    window_id: int | None
    source_flags: int | None
    slot: int
    old_item: ItemInstance
    new_item: ItemInstance


class _InventoryActionType(Type["InventoryAction"]):
    """InventoryAction compound type with conditional fields."""

    def read(self, reader: PacketReader) -> InventoryAction:
        source_type = UVAR_INT.read(reader)  # SourceType
        window_id: int | None = None
        source_flags: int | None = None
        if source_type in (InventorySourceType.CONTAINER_INVENTORY, InventorySourceType.NON_IMPLEMENTED_FEATURE_TODO):
            window_id = VAR_INT.read(reader)  # WindowID
        elif source_type == InventorySourceType.WORLD_INTERACTION:
            source_flags = UVAR_INT.read(reader)  # SourceFlags
        slot = UVAR_INT.read(reader)  # InventorySlot
        old_item = ITEM_INSTANCE.read(reader)  # OldItem
        new_item = ITEM_INSTANCE.read(reader)  # NewItem
        return InventoryAction(source_type, window_id, source_flags, slot, old_item, new_item)

    def write(self, writer: PacketWriter, value: InventoryAction) -> None:
        UVAR_INT.write(writer, value.source_type)  # SourceType
        if value.source_type in (
            InventorySourceType.CONTAINER_INVENTORY,
            InventorySourceType.NON_IMPLEMENTED_FEATURE_TODO,
        ):
            assert value.window_id is not None
            VAR_INT.write(writer, value.window_id)  # WindowID
        elif value.source_type == InventorySourceType.WORLD_INTERACTION:
            assert value.source_flags is not None
            UVAR_INT.write(writer, value.source_flags)  # SourceFlags
        UVAR_INT.write(writer, value.slot)  # InventorySlot
        ITEM_INSTANCE.write(writer, value.old_item)  # OldItem
        ITEM_INSTANCE.write(writer, value.new_item)  # NewItem


INVENTORY_ACTION = _InventoryActionType()
