/// ChangeValue codec implementation.

#include "endweave/codec/types/data_store.h"

namespace endweave {

std::expected<ChangeValue, ReadError> read_change_value(PacketReader& reader) {
    auto type_id = reader.read_uvarint();
    if (!type_id) return std::unexpected(type_id.error());

    ChangeValue cv;
    cv.type_id = *type_id;

    switch (*type_id) {
    case 0: break; // empty
    case 1: { auto v = reader.read_bool(); if (!v) return std::unexpected(v.error()); cv.bool_value = *v; break; }
    case 2: { auto v = reader.read_int64_le(); if (!v) return std::unexpected(v.error()); cv.int64_value = *v; break; }
    case 4: { auto v = reader.read_string(); if (!v) return std::unexpected(v.error()); cv.string_value = std::move(*v); break; }
    case 6: {
        auto count = reader.read_uvarint();
        if (!count) return std::unexpected(count.error());
        cv.entries.reserve(*count);
        for (std::uint32_t i = 0; i < *count; ++i) {
            auto key = reader.read_string();
            if (!key) return std::unexpected(key.error());
            auto val = read_change_value(reader);
            if (!val) return std::unexpected(val.error());
            cv.entries.emplace_back(std::move(*key), std::move(*val));
        }
        break;
    }
    default: return std::unexpected(ReadError::out_of_bounds);
    }
    return cv;
}

void write_change_value(PacketWriter& writer, const ChangeValue& cv) {
    writer.write_uvarint(cv.type_id);
    switch (cv.type_id) {
    case 0: break;
    case 1: writer.write_bool(cv.bool_value); break;
    case 2: writer.write_int64_le(cv.int64_value); break;
    case 4: writer.write_string(cv.string_value); break;
    case 6:
        writer.write_uvarint(static_cast<std::uint32_t>(cv.entries.size()));
        for (auto& [key, val] : cv.entries) {
            writer.write_string(key);
            write_change_value(writer, val);
        }
        break;
    default: break;
    }
}

template <> auto PacketReader::read<change_value>() -> std::expected<ChangeValue, ReadError> {
    return read_change_value(*this);
}
template <> void PacketWriter::write<change_value>(const ChangeValue& val) {
    write_change_value(*this, val);
}

} // namespace endweave
