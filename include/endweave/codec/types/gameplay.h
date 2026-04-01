#pragma once

/// GameRule, Experiment compound types.

#include <cstdint>
#include <expected>
#include <string>
#include <variant>
#include <vector>

#include "endweave/codec/reader.h"
#include "endweave/codec/writer.h"

namespace endweave {

struct GameRule {
    std::string name;
    bool editable = false;
    std::uint32_t type_id = 0; // 1=bool, 2=int, 3=float
    std::variant<bool, std::int32_t, float> value;
};

struct Experiment {
    std::string name;
    bool enabled = false;
};

// GameRules: varint count + entries
struct game_rules { using value_type = std::vector<GameRule>; };
// Experiments: uint32 count + entries (v924+)
struct experiments { using value_type = std::vector<Experiment>; };
// Experiments v860: int32 count + entries
struct experiments_v860 { using value_type = std::vector<Experiment>; };

template <> auto PacketReader::read<game_rules>() -> std::expected<std::vector<GameRule>, ReadError>;
template <> void PacketWriter::write<game_rules>(const std::vector<GameRule>& val);
template <> auto PacketReader::read<experiments>() -> std::expected<std::vector<Experiment>, ReadError>;
template <> void PacketWriter::write<experiments>(const std::vector<Experiment>& val);
template <> auto PacketReader::read<experiments_v860>() -> std::expected<std::vector<Experiment>, ReadError>;
template <> void PacketWriter::write<experiments_v860>(const std::vector<Experiment>& val);

} // namespace endweave
