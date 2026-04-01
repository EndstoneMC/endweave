/// GameRule, Experiment codec implementation.

#include "endweave/codec/types/gameplay.h"
#include "endweave/codec/types/primitives.h"

namespace endweave {

template <> auto PacketReader::read<game_rules>() -> std::expected<std::vector<GameRule>, ReadError> {
    auto count = read_uvarint();
    if (!count) return std::unexpected(count.error());
    std::vector<GameRule> rules;
    rules.reserve(*count);
    for (std::uint32_t i = 0; i < *count; ++i) {
        GameRule r;
        auto name = read_string();
        if (!name) return std::unexpected(name.error());
        r.name = std::move(*name);
        auto editable = read_bool();
        if (!editable) return std::unexpected(editable.error());
        r.editable = *editable;
        auto type_id = read_uvarint();
        if (!type_id) return std::unexpected(type_id.error());
        r.type_id = *type_id;
        switch (*type_id) {
        case 1: { auto v = read_bool(); if (!v) return std::unexpected(v.error()); r.value = *v; break; }
        case 2: { auto v = read_varint(); if (!v) return std::unexpected(v.error()); r.value = *v; break; }
        case 3: { auto v = read_float_le(); if (!v) return std::unexpected(v.error()); r.value = *v; break; }
        default: return std::unexpected(ReadError::out_of_bounds);
        }
        rules.push_back(std::move(r));
    }
    return rules;
}

template <> void PacketWriter::write<game_rules>(const std::vector<GameRule>& val) {
    write_uvarint(static_cast<std::uint32_t>(val.size()));
    for (auto& r : val) {
        write_string(r.name);
        write_bool(r.editable);
        write_uvarint(r.type_id);
        switch (r.type_id) {
        case 1: write_bool(std::get<bool>(r.value)); break;
        case 2: write_varint(std::get<std::int32_t>(r.value)); break;
        case 3: write_float_le(std::get<float>(r.value)); break;
        default: break;
        }
    }
}

namespace {
template <typename CountTag>
std::expected<std::vector<Experiment>, ReadError> read_experiments_impl(PacketReader& reader) {
    auto count_result = reader.read<CountTag>();
    if (!count_result) return std::unexpected(count_result.error());
    auto count = static_cast<std::uint32_t>(*count_result);
    std::vector<Experiment> exps;
    exps.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        Experiment e;
        auto name = reader.read_string();
        if (!name) return std::unexpected(name.error());
        e.name = std::move(*name);
        auto enabled = reader.read_bool();
        if (!enabled) return std::unexpected(enabled.error());
        e.enabled = *enabled;
        exps.push_back(std::move(e));
    }
    return exps;
}

template <typename CountTag>
void write_experiments_impl(PacketWriter& writer, const std::vector<Experiment>& val) {
    writer.write<CountTag>(static_cast<typename CountTag::value_type>(val.size()));
    for (auto& e : val) {
        writer.write_string(e.name);
        writer.write_bool(e.enabled);
    }
}
} // namespace

template <> auto PacketReader::read<experiments>() -> std::expected<std::vector<Experiment>, ReadError> {
    return read_experiments_impl<uint_le>(*this);
}
template <> void PacketWriter::write<experiments>(const std::vector<Experiment>& val) {
    write_experiments_impl<uint_le>(*this, val);
}
template <> auto PacketReader::read<experiments_v860>() -> std::expected<std::vector<Experiment>, ReadError> {
    return read_experiments_impl<int_le>(*this);
}
template <> void PacketWriter::write<experiments_v860>(const std::vector<Experiment>& val) {
    write_experiments_impl<int_le>(*this, val);
}

} // namespace endweave
