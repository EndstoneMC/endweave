#pragma once

/// InventoryAction compound type.

#include <cstdint>
#include <expected>
#include <optional>

#include "endweave/codec/reader.h"
#include "endweave/codec/types/item.h"
#include "endweave/codec/writer.h"

namespace endweave {

struct InventoryAction {
    std::uint32_t source_type = 0;
    std::optional<std::int32_t> window_id;
    std::optional<std::uint32_t> source_flags;
    std::uint32_t slot = 0;
    ItemInstance old_item;
    ItemInstance new_item;
};

std::expected<InventoryAction, ReadError> read_inventory_action(PacketReader& reader);
void write_inventory_action(PacketWriter& writer, const InventoryAction& action);

struct inventory_action { using value_type = InventoryAction; };
template <> auto PacketReader::read<inventory_action>() -> std::expected<InventoryAction, ReadError>;
template <> void PacketWriter::write<inventory_action>(const InventoryAction& val);

} // namespace endweave
