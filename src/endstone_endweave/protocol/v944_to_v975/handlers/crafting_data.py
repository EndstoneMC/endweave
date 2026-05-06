"""CraftingDataPacket (52) -- v944 server to v975 client.

v975 removed CraftingDataEntryType::FurnaceRecipe and FurnaceAuxRecipe.
Furnace entries are stripped from the v944 server's packet before forwarding.
"""

from dataclasses import dataclass

from ....codec import ArrayType, CraftingDataEntryType, PacketWrapper
from ....codec.reader import PacketReader
from ....codec.types import Type
from ....codec.writer import PacketWriter


@dataclass
class CraftingDataEntry:
    """Deserialized CraftingDataEntry (one recipe in a CraftingDataPacket).

    Attributes:
        type: Crafting Type discriminator selecting the per-type payload.
    """

    type: CraftingDataEntryType


class _CraftingDataEntryType(Type[CraftingDataEntry]):
    """CraftingDataEntry codec."""

    def read(self, reader: PacketReader) -> CraftingDataEntry:
        type_ = CraftingDataEntryType(reader.read_varint())
        return CraftingDataEntry(type=type_)

    def write(self, writer: PacketWriter, value: CraftingDataEntry) -> None:
        writer.write_varint(value.type.value)


CRAFTING_DATA_ENTRY = _CraftingDataEntryType()


def rewrite_crafting_data(wrapper: PacketWrapper) -> None:
    """Strip FurnaceRecipe and FurnaceAuxRecipe entries from the recipe list."""
    entries = wrapper.read(ArrayType(CRAFTING_DATA_ENTRY))  # Crafting Entries
    kept = [
        e
        for e in entries
        if e.type
        not in (
            CraftingDataEntryType.FURNACE_RECIPE,
            CraftingDataEntryType.FURNACE_AUX_RECIPE,
        )
    ]
    wrapper.write(ArrayType(CRAFTING_DATA_ENTRY), kept)
    wrapper.passthrough_all()
