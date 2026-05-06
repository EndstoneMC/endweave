"""CraftingDataPacket (52) -- v944 server to v975 client.

v975 removed CraftingDataEntryType::FurnaceRecipe and FurnaceAuxRecipe.
Furnace entries are stripped from the v944 server's packet before forwarding.
"""

from endstone_endweave.codec import (
    CRAFTING_DATA_ENTRY,
    ArrayType,
    CraftingDataEntryType,
    PacketWrapper,
)


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
