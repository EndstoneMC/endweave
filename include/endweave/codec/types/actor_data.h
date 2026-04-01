#pragma once

/// ActorData (entity metadata) compound type.
///
/// See Also:
///     com.viaversion.viaversion.api.type.types.entitydata.EntityDataListType

#include <any>
#include <cstdint>
#include <expected>
#include <vector>

#include "endweave/codec/reader.h"
#include "endweave/codec/writer.h"

namespace endweave {

struct ActorDataItem {
    std::uint32_t key = 0;
    std::uint32_t type_id = 0;
    std::any value;
};

/// Read a single ActorData entry.
std::expected<ActorDataItem, ReadError> read_actor_data_item(PacketReader& reader);

/// Write a single ActorData entry.
void write_actor_data_item(PacketWriter& writer, const ActorDataItem& item);

/// Read a uvarint-prefixed list of ActorData entries.
std::expected<std::vector<ActorDataItem>, ReadError> read_actor_data_list(PacketReader& reader);

/// Write a uvarint-prefixed list of ActorData entries.
void write_actor_data_list(PacketWriter& writer, const std::vector<ActorDataItem>& items);

// Type tags for PacketWrapper
struct actor_data_item { using value_type = ActorDataItem; };
struct actor_data_list { using value_type = std::vector<ActorDataItem>; };

template <> auto PacketReader::read<actor_data_item>() -> std::expected<ActorDataItem, ReadError>;
template <> auto PacketReader::read<actor_data_list>() -> std::expected<std::vector<ActorDataItem>, ReadError>;
template <> void PacketWriter::write<actor_data_item>(const ActorDataItem& val);
template <> void PacketWriter::write<actor_data_list>(const std::vector<ActorDataItem>& val);

} // namespace endweave
