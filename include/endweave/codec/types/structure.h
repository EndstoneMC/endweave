#pragma once

/// StructureSettings compound types (v924 and v944 variants).

#include <cstdint>
#include <expected>
#include <string>
#include <tuple>

#include "endweave/codec/reader.h"
#include "endweave/codec/types/primitives.h"
#include "endweave/codec/writer.h"

namespace endweave {

struct StructureSettings {
    std::string palette_name;
    bool ignore_entities = false;
    bool ignore_blocks = false;
    bool allow_non_ticking_chunks = false;
    BlockPosValue size;
    BlockPosValue offset;
    std::int64_t last_edit_player = 0;
    std::uint8_t rotation = 0;
    std::uint8_t mirror = 0;
    std::uint8_t animation_mode = 0;
    float animation_seconds = 0.0f;
    float integrity_value = 0.0f;
    std::uint32_t integrity_seed = 0;
    Vec3Value pivot;
};

/// v924: Size/Offset as NetworkBlockPos (uvarint Y)
struct structure_settings_v924 { using value_type = StructureSettings; };
/// v944: Size/Offset as BlockPos (varint Y)
struct structure_settings_v944 { using value_type = StructureSettings; };

template <> auto PacketReader::read<structure_settings_v924>() -> std::expected<StructureSettings, ReadError>;
template <> auto PacketReader::read<structure_settings_v944>() -> std::expected<StructureSettings, ReadError>;
template <> void PacketWriter::write<structure_settings_v924>(const StructureSettings& val);
template <> void PacketWriter::write<structure_settings_v944>(const StructureSettings& val);

} // namespace endweave
