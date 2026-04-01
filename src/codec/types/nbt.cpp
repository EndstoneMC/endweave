/// Bedrock network NBT codec implementation.

#include "endweave/codec/types/nbt.h"

namespace endweave::nbt {

namespace {

std::expected<std::string, ReadError> read_nbt_string(PacketReader& reader) {
    auto length = reader.read_uvarint();
    if (!length) return std::unexpected(length.error());
    auto bytes = reader.read_bytes(*length);
    if (!bytes) return std::unexpected(bytes.error());
    return std::string(bytes->data(), bytes->size());
}

void write_nbt_string(PacketWriter& writer, const std::string& value) {
    writer.write_uvarint(static_cast<std::uint32_t>(value.size()));
    writer.write_bytes(value);
}

} // namespace

std::expected<std::unique_ptr<Tag>, ReadError> read_value(PacketReader& reader, int tag_type, int depth) {
    if (depth > MAX_DEPTH) return std::unexpected(ReadError::varint_too_long); // depth exceeded

    switch (tag_type) {
    case TAG_BYTE: {
        auto v = reader.read_byte();
        if (!v) return std::unexpected(v.error());
        return std::make_unique<ByteTag>(static_cast<std::int8_t>(*v));
    }
    case TAG_SHORT: {
        auto v = reader.read_short_le();
        if (!v) return std::unexpected(v.error());
        return std::make_unique<ShortTag>(*v);
    }
    case TAG_INT: {
        auto v = reader.read_varint();
        if (!v) return std::unexpected(v.error());
        return std::make_unique<IntTag>(*v);
    }
    case TAG_LONG: {
        auto v = reader.read_varint64();
        if (!v) return std::unexpected(v.error());
        return std::make_unique<LongTag>(*v);
    }
    case TAG_FLOAT: {
        auto v = reader.read_float_le();
        if (!v) return std::unexpected(v.error());
        return std::make_unique<FloatTag>(*v);
    }
    case TAG_DOUBLE: {
        auto v = reader.read_double_le();
        if (!v) return std::unexpected(v.error());
        return std::make_unique<DoubleTag>(*v);
    }
    case TAG_BYTE_ARRAY: {
        auto len = reader.read_varint();
        if (!len) return std::unexpected(len.error());
        auto bytes = reader.read_bytes(static_cast<std::size_t>(*len));
        if (!bytes) return std::unexpected(bytes.error());
        return std::make_unique<ByteArrayTag>(
            std::vector<std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(bytes->data()),
                reinterpret_cast<const std::uint8_t*>(bytes->data() + bytes->size())
            )
        );
    }
    case TAG_STRING: {
        auto v = read_nbt_string(reader);
        if (!v) return std::unexpected(v.error());
        return std::make_unique<StringTag>(std::move(*v));
    }
    case TAG_LIST: {
        auto elem_type = reader.read_byte();
        if (!elem_type) return std::unexpected(elem_type.error());
        auto count = reader.read_varint();
        if (!count) return std::unexpected(count.error());
        std::vector<std::unique_ptr<Tag>> tags;
        if (*elem_type != TAG_END && *count > 0) {
            tags.reserve(static_cast<std::size_t>(*count));
            for (std::int32_t i = 0; i < *count; ++i) {
                auto child = read_value(reader, static_cast<int>(*elem_type), depth + 1);
                if (!child) return std::unexpected(child.error());
                tags.push_back(std::move(*child));
            }
        }
        return std::make_unique<ListTag>(static_cast<int>(*elem_type), std::move(tags));
    }
    case TAG_COMPOUND: {
        auto result = std::make_unique<CompoundTag>();
        while (true) {
            auto child_type = reader.read_byte();
            if (!child_type) return std::unexpected(child_type.error());
            if (*child_type == TAG_END) break;
            auto name = read_nbt_string(reader);
            if (!name) return std::unexpected(name.error());
            auto child = read_value(reader, static_cast<int>(*child_type), depth + 1);
            if (!child) return std::unexpected(child.error());
            result->value[std::move(*name)] = std::move(*child);
        }
        return result;
    }
    case TAG_INT_ARRAY: {
        auto count = reader.read_varint();
        if (!count) return std::unexpected(count.error());
        std::vector<std::int32_t> values;
        values.reserve(static_cast<std::size_t>(*count));
        for (std::int32_t i = 0; i < *count; ++i) {
            auto v = reader.read_varint();
            if (!v) return std::unexpected(v.error());
            values.push_back(*v);
        }
        return std::make_unique<IntArrayTag>(std::move(values));
    }
    case TAG_LONG_ARRAY: {
        auto count = reader.read_varint();
        if (!count) return std::unexpected(count.error());
        std::vector<std::int64_t> values;
        values.reserve(static_cast<std::size_t>(*count));
        for (std::int32_t i = 0; i < *count; ++i) {
            auto v = reader.read_varint64();
            if (!v) return std::unexpected(v.error());
            values.push_back(*v);
        }
        return std::make_unique<LongArrayTag>(std::move(values));
    }
    default:
        return std::unexpected(ReadError::out_of_bounds); // unknown tag type
    }
}

std::expected<std::unique_ptr<CompoundTag>, ReadError> read_nbt(PacketReader& reader, bool read_name) {
    auto root_type = reader.read_byte();
    if (!root_type) return std::unexpected(root_type.error());

    if (*root_type == TAG_END) return nullptr;
    if (*root_type != TAG_COMPOUND) return std::unexpected(ReadError::out_of_bounds);

    if (read_name) {
        auto name = read_nbt_string(reader);
        if (!name) return std::unexpected(name.error());
        // discard root name
    }

    auto result = std::make_unique<CompoundTag>();
    while (true) {
        auto child_type = reader.read_byte();
        if (!child_type) return std::unexpected(child_type.error());
        if (*child_type == TAG_END) break;
        auto name = read_nbt_string(reader);
        if (!name) return std::unexpected(name.error());
        auto child = read_value(reader, static_cast<int>(*child_type), 1);
        if (!child) return std::unexpected(child.error());
        result->value[std::move(*name)] = std::move(*child);
    }
    return result;
}

void write_value(PacketWriter& writer, const Tag& tag) {
    switch (tag.tag_id()) {
    case TAG_BYTE:
        writer.write_byte(static_cast<std::uint8_t>(static_cast<const ByteTag&>(tag).value));
        break;
    case TAG_SHORT:
        writer.write_short_le(static_cast<const ShortTag&>(tag).value);
        break;
    case TAG_INT:
        writer.write_varint(static_cast<const IntTag&>(tag).value);
        break;
    case TAG_LONG:
        writer.write_varint64(static_cast<const LongTag&>(tag).value);
        break;
    case TAG_FLOAT:
        writer.write_float_le(static_cast<const FloatTag&>(tag).value);
        break;
    case TAG_DOUBLE:
        writer.write_double_le(static_cast<const DoubleTag&>(tag).value);
        break;
    case TAG_BYTE_ARRAY: {
        auto& arr = static_cast<const ByteArrayTag&>(tag);
        writer.write_varint(static_cast<std::int32_t>(arr.value.size()));
        writer.write_bytes({reinterpret_cast<const char*>(arr.value.data()), arr.value.size()});
        break;
    }
    case TAG_STRING:
        write_nbt_string(writer, static_cast<const StringTag&>(tag).value);
        break;
    case TAG_LIST: {
        auto& list = static_cast<const ListTag&>(tag);
        writer.write_byte(static_cast<std::uint8_t>(list.element_type));
        writer.write_varint(static_cast<std::int32_t>(list.tags.size()));
        for (auto& child : list.tags) write_value(writer, *child);
        break;
    }
    case TAG_COMPOUND: {
        auto& compound = static_cast<const CompoundTag&>(tag);
        for (auto& [name, child] : compound.value) {
            writer.write_byte(static_cast<std::uint8_t>(child->tag_id()));
            write_nbt_string(writer, name);
            write_value(writer, *child);
        }
        writer.write_byte(TAG_END);
        break;
    }
    case TAG_INT_ARRAY: {
        auto& arr = static_cast<const IntArrayTag&>(tag);
        writer.write_varint(static_cast<std::int32_t>(arr.value.size()));
        for (auto v : arr.value) writer.write_varint(v);
        break;
    }
    case TAG_LONG_ARRAY: {
        auto& arr = static_cast<const LongArrayTag&>(tag);
        writer.write_varint(static_cast<std::int32_t>(arr.value.size()));
        for (auto v : arr.value) writer.write_varint64(v);
        break;
    }
    default:
        break;
    }
}

void write_nbt(PacketWriter& writer, const CompoundTag* tag, const char* name) {
    if (!tag) {
        writer.write_byte(TAG_END);
        return;
    }
    writer.write_byte(TAG_COMPOUND);
    if (name) write_nbt_string(writer, name);
    write_value(writer, *tag);
}

} // namespace endweave::nbt

// -- Type tag specializations --

namespace endweave {

template <>
auto PacketReader::read<named_compound_tag>() -> std::expected<std::unique_ptr<nbt::CompoundTag>, ReadError> {
    return nbt::read_nbt(*this, true);
}

template <>
auto PacketReader::read<compound_tag>() -> std::expected<std::unique_ptr<nbt::CompoundTag>, ReadError> {
    return nbt::read_nbt(*this, false);
}

template <>
void PacketWriter::write<named_compound_tag>(const std::unique_ptr<nbt::CompoundTag>& val) {
    nbt::write_nbt(*this, val.get(), "");
}

template <>
void PacketWriter::write<compound_tag>(const std::unique_ptr<nbt::CompoundTag>& val) {
    nbt::write_nbt(*this, val.get(), nullptr);
}

} // namespace endweave
