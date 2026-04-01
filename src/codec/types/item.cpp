/// ItemInstance codec implementation.

#include "endweave/codec/types/item.h"

namespace endweave {

std::expected<ItemInstance, ReadError> read_item_instance(PacketReader& reader) {
    auto network_id = reader.read_varint();
    if (!network_id) return std::unexpected(network_id.error());

    if (*network_id == 0) return ItemInstance{};

    ItemInstance item;
    item.network_id = *network_id;

    auto count = reader.read_ushort_le();
    if (!count) return std::unexpected(count.error());
    item.count = *count;

    auto aux = reader.read_uvarint();
    if (!aux) return std::unexpected(aux.error());
    item.aux_value = *aux;

    auto has_net = reader.read_bool();
    if (!has_net) return std::unexpected(has_net.error());
    item.has_net_id = *has_net;

    if (item.has_net_id) {
        auto sni = reader.read_varint();
        if (!sni) return std::unexpected(sni.error());
        item.stack_net_id = *sni;
    }

    auto brid = reader.read_varint();
    if (!brid) return std::unexpected(brid.error());
    item.block_runtime_id = *brid;

    auto extra_len = reader.read_uvarint();
    if (!extra_len) return std::unexpected(extra_len.error());

    auto extra = reader.read_bytes(static_cast<std::size_t>(*extra_len));
    if (!extra) return std::unexpected(extra.error());
    item.user_data = std::string(extra->data(), extra->size());

    return item;
}

void write_item_instance(PacketWriter& writer, const ItemInstance& item) {
    writer.write_varint(item.network_id);
    if (item.network_id == 0) return;

    writer.write_ushort_le(item.count);
    writer.write_uvarint(item.aux_value);
    writer.write_bool(item.has_net_id);
    if (item.has_net_id) writer.write_varint(item.stack_net_id);
    writer.write_varint(item.block_runtime_id);
    writer.write_uvarint(static_cast<std::uint32_t>(item.user_data.size()));
    writer.write_bytes(item.user_data);
}

template <> auto PacketReader::read<item_instance>() -> std::expected<ItemInstance, ReadError> {
    return read_item_instance(*this);
}
template <> void PacketWriter::write<item_instance>(const ItemInstance& val) {
    write_item_instance(*this, val);
}

} // namespace endweave
