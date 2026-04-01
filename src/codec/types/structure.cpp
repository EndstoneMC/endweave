/// StructureSettings codec implementation.

#include "endweave/codec/types/structure.h"

namespace endweave {

namespace {

template <typename PosTag>
std::expected<StructureSettings, ReadError> read_settings(PacketReader& reader) {
    StructureSettings s;
    auto palette = reader.read_string();
    if (!palette) return std::unexpected(palette.error());
    s.palette_name = std::move(*palette);

    auto ignore_ent = reader.read_bool();
    if (!ignore_ent) return std::unexpected(ignore_ent.error());
    s.ignore_entities = *ignore_ent;

    auto ignore_blk = reader.read_bool();
    if (!ignore_blk) return std::unexpected(ignore_blk.error());
    s.ignore_blocks = *ignore_blk;

    auto allow_ntc = reader.read_bool();
    if (!allow_ntc) return std::unexpected(allow_ntc.error());
    s.allow_non_ticking_chunks = *allow_ntc;

    auto size = reader.read<PosTag>();
    if (!size) return std::unexpected(size.error());
    s.size = *size;

    auto offset = reader.read<PosTag>();
    if (!offset) return std::unexpected(offset.error());
    s.offset = *offset;

    auto player = reader.read_varint64();
    if (!player) return std::unexpected(player.error());
    s.last_edit_player = *player;

    auto rot = reader.read_byte();
    if (!rot) return std::unexpected(rot.error());
    s.rotation = *rot;

    auto mir = reader.read_byte();
    if (!mir) return std::unexpected(mir.error());
    s.mirror = *mir;

    auto anim_mode = reader.read_byte();
    if (!anim_mode) return std::unexpected(anim_mode.error());
    s.animation_mode = *anim_mode;

    auto anim_sec = reader.read_float_le();
    if (!anim_sec) return std::unexpected(anim_sec.error());
    s.animation_seconds = *anim_sec;

    auto integrity = reader.read_float_le();
    if (!integrity) return std::unexpected(integrity.error());
    s.integrity_value = *integrity;

    auto seed = reader.read_uint_le();
    if (!seed) return std::unexpected(seed.error());
    s.integrity_seed = *seed;

    auto pivot = reader.read<vec3>();
    if (!pivot) return std::unexpected(pivot.error());
    s.pivot = *pivot;

    return s;
}

template <typename PosTag>
void write_settings(PacketWriter& writer, const StructureSettings& s) {
    writer.write_string(s.palette_name);
    writer.write_bool(s.ignore_entities);
    writer.write_bool(s.ignore_blocks);
    writer.write_bool(s.allow_non_ticking_chunks);
    writer.write<PosTag>(s.size);
    writer.write<PosTag>(s.offset);
    writer.write_varint64(s.last_edit_player);
    writer.write_byte(s.rotation);
    writer.write_byte(s.mirror);
    writer.write_byte(s.animation_mode);
    writer.write_float_le(s.animation_seconds);
    writer.write_float_le(s.integrity_value);
    writer.write_uint_le(s.integrity_seed);
    writer.write<vec3>(s.pivot);
}

} // namespace

template <> auto PacketReader::read<structure_settings_v924>() -> std::expected<StructureSettings, ReadError> {
    return read_settings<network_block_pos>(*this);
}
template <> auto PacketReader::read<structure_settings_v944>() -> std::expected<StructureSettings, ReadError> {
    return read_settings<block_pos>(*this);
}
template <> void PacketWriter::write<structure_settings_v924>(const StructureSettings& val) {
    write_settings<network_block_pos>(*this, val);
}
template <> void PacketWriter::write<structure_settings_v944>(const StructureSettings& val) {
    write_settings<block_pos>(*this, val);
}

} // namespace endweave
