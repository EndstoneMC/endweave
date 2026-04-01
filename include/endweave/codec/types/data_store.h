#pragma once

/// ChangeValue recursive data store type.

#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "endweave/codec/reader.h"
#include "endweave/codec/writer.h"

namespace endweave {

struct ChangeValue {
    std::uint32_t type_id = 0; // 0=empty, 1=bool, 2=int64, 4=string, 6=map
    bool bool_value = false;
    std::int64_t int64_value = 0;
    std::string string_value;
    std::vector<std::pair<std::string, ChangeValue>> entries; // for map type
};

std::expected<ChangeValue, ReadError> read_change_value(PacketReader& reader);
void write_change_value(PacketWriter& writer, const ChangeValue& val);

struct change_value { using value_type = ChangeValue; };
template <> auto PacketReader::read<change_value>() -> std::expected<ChangeValue, ReadError>;
template <> void PacketWriter::write<change_value>(const ChangeValue& val);

} // namespace endweave
