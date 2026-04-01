/// InventoryAction codec implementation.

#include "endweave/codec/types/inventory.h"
#include "endweave/codec/types/enums.h"

namespace endweave {

std::expected<InventoryAction, ReadError> read_inventory_action(PacketReader& reader) {
    auto source_type = reader.read_uvarint();
    if (!source_type) return std::unexpected(source_type.error());

    InventoryAction action;
    action.source_type = *source_type;

    if (*source_type == static_cast<std::uint32_t>(InventorySourceType::ContainerInventory) ||
        *source_type == static_cast<std::uint32_t>(InventorySourceType::NonImplementedFeatureTODO)) {
        auto wid = reader.read_varint();
        if (!wid) return std::unexpected(wid.error());
        action.window_id = *wid;
    } else if (*source_type == static_cast<std::uint32_t>(InventorySourceType::WorldInteraction)) {
        auto flags = reader.read_uvarint();
        if (!flags) return std::unexpected(flags.error());
        action.source_flags = *flags;
    }

    auto slot = reader.read_uvarint();
    if (!slot) return std::unexpected(slot.error());
    action.slot = *slot;

    auto old_item = read_item_instance(reader);
    if (!old_item) return std::unexpected(old_item.error());
    action.old_item = std::move(*old_item);

    auto new_item = read_item_instance(reader);
    if (!new_item) return std::unexpected(new_item.error());
    action.new_item = std::move(*new_item);

    return action;
}

void write_inventory_action(PacketWriter& writer, const InventoryAction& action) {
    writer.write_uvarint(action.source_type);

    if (action.source_type == static_cast<std::uint32_t>(InventorySourceType::ContainerInventory) ||
        action.source_type == static_cast<std::uint32_t>(InventorySourceType::NonImplementedFeatureTODO)) {
        writer.write_varint(action.window_id.value_or(0));
    } else if (action.source_type == static_cast<std::uint32_t>(InventorySourceType::WorldInteraction)) {
        writer.write_uvarint(action.source_flags.value_or(0));
    }

    writer.write_uvarint(action.slot);
    write_item_instance(writer, action.old_item);
    write_item_instance(writer, action.new_item);
}

template <> auto PacketReader::read<inventory_action>() -> std::expected<InventoryAction, ReadError> {
    return read_inventory_action(*this);
}
template <> void PacketWriter::write<inventory_action>(const InventoryAction& val) {
    write_inventory_action(*this, val);
}

} // namespace endweave
