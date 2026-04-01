#pragma once

/// ItemInstance codec type.

#include <cstdint>
#include <expected>
#include <string>

#include "endweave/codec/reader.h"
#include "endweave/codec/writer.h"

namespace endweave {

struct ItemInstance {
    std::int32_t network_id = 0;
    std::uint16_t count = 0;
    std::uint32_t aux_value = 0;
    bool has_net_id = false;
    std::int32_t stack_net_id = 0;
    std::int32_t block_runtime_id = 0;
    std::string user_data;
};

std::expected<ItemInstance, ReadError> read_item_instance(PacketReader& reader);
void write_item_instance(PacketWriter& writer, const ItemInstance& item);

// Type tag for PacketWrapper
struct item_instance { using value_type = ItemInstance; };

template <> auto PacketReader::read<item_instance>() -> std::expected<ItemInstance, ReadError>;
template <> void PacketWriter::write<item_instance>(const ItemInstance& val);

} // namespace endweave
