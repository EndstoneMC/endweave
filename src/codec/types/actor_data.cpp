/// ActorData codec implementation.

#include "endweave/codec/types/actor_data.h"

#include <memory>

#include "endweave/codec/types/nbt.h"
#include "endweave/codec/types/primitives.h"

namespace endweave {

std::expected<ActorDataItem, ReadError> read_actor_data_item(PacketReader& reader) {
    auto key = reader.read_uvarint();
    if (!key) return std::unexpected(key.error());
    auto type_id = reader.read_uvarint();
    if (!type_id) return std::unexpected(type_id.error());

    ActorDataItem item;
    item.key = *key;
    item.type_id = *type_id;

    switch (*type_id) {
    case 0: { // Byte
        auto v = reader.read_byte();
        if (!v) return std::unexpected(v.error());
        item.value = static_cast<std::int32_t>(*v);
        break;
    }
    case 1: { // Short
        auto v = reader.read_short_le();
        if (!v) return std::unexpected(v.error());
        item.value = static_cast<std::int32_t>(*v);
        break;
    }
    case 2: { // Int (varint)
        auto v = reader.read_varint();
        if (!v) return std::unexpected(v.error());
        item.value = *v;
        break;
    }
    case 3: { // Float
        auto v = reader.read_float_le();
        if (!v) return std::unexpected(v.error());
        item.value = *v;
        break;
    }
    case 4: { // String
        auto v = reader.read_string();
        if (!v) return std::unexpected(v.error());
        item.value = std::move(*v);
        break;
    }
    case 5: { // CompoundTag (named NBT)
        auto v = nbt::read_nbt(reader, true);
        if (!v) return std::unexpected(v.error());
        // Use shared_ptr because std::any requires copyable types
        item.value = std::shared_ptr<nbt::CompoundTag>(std::move(*v));
        break;
    }
    case 6: { // BlockPos (varint x, varint y, varint z)
        auto v = reader.read<block_pos>();
        if (!v) return std::unexpected(v.error());
        item.value = *v;
        break;
    }
    case 7: { // Int64 (varint64)
        auto v = reader.read_varint64();
        if (!v) return std::unexpected(v.error());
        item.value = *v;
        break;
    }
    case 8: { // Vec3
        auto v = reader.read<vec3>();
        if (!v) return std::unexpected(v.error());
        item.value = *v;
        break;
    }
    default:
        return std::unexpected(ReadError::out_of_bounds);
    }

    return item;
}

void write_actor_data_item(PacketWriter& writer, const ActorDataItem& item) {
    writer.write_uvarint(item.key);
    writer.write_uvarint(item.type_id);

    switch (item.type_id) {
    case 0: // Byte
        writer.write_byte(static_cast<std::uint8_t>(std::any_cast<std::int32_t>(item.value)));
        break;
    case 1: // Short
        writer.write_short_le(static_cast<std::int16_t>(std::any_cast<std::int32_t>(item.value)));
        break;
    case 2: // Int
        writer.write_varint(std::any_cast<std::int32_t>(item.value));
        break;
    case 3: // Float
        writer.write_float_le(std::any_cast<float>(item.value));
        break;
    case 4: // String
        writer.write_string(std::any_cast<const std::string&>(item.value));
        break;
    case 5: { // CompoundTag
        auto& tag = std::any_cast<const std::shared_ptr<nbt::CompoundTag>&>(item.value);
        nbt::write_nbt(writer, tag.get(), "");
        break;
    }
    case 6: // BlockPos
        writer.write<block_pos>(std::any_cast<BlockPosValue>(item.value));
        break;
    case 7: // Int64
        writer.write_varint64(std::any_cast<std::int64_t>(item.value));
        break;
    case 8: // Vec3
        writer.write<vec3>(std::any_cast<Vec3Value>(item.value));
        break;
    default:
        break;
    }
}

std::expected<std::vector<ActorDataItem>, ReadError> read_actor_data_list(PacketReader& reader) {
    auto count = reader.read_uvarint();
    if (!count) return std::unexpected(count.error());
    std::vector<ActorDataItem> items;
    items.reserve(*count);
    for (std::uint32_t i = 0; i < *count; ++i) {
        auto item = read_actor_data_item(reader);
        if (!item) return std::unexpected(item.error());
        items.push_back(std::move(*item));
    }
    return items;
}

void write_actor_data_list(PacketWriter& writer, const std::vector<ActorDataItem>& items) {
    writer.write_uvarint(static_cast<std::uint32_t>(items.size()));
    for (auto& item : items) write_actor_data_item(writer, item);
}

// Type tag specializations
template <> auto PacketReader::read<actor_data_item>() -> std::expected<ActorDataItem, ReadError> {
    return read_actor_data_item(*this);
}
template <> auto PacketReader::read<actor_data_list>() -> std::expected<std::vector<ActorDataItem>, ReadError> {
    return read_actor_data_list(*this);
}
template <> void PacketWriter::write<actor_data_item>(const ActorDataItem& val) {
    write_actor_data_item(*this, val);
}
template <> void PacketWriter::write<actor_data_list>(const std::vector<ActorDataItem>& val) {
    write_actor_data_list(*this, val);
}

} // namespace endweave
