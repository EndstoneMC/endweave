#pragma once

/// Command-related compound types for AvailableCommandsPacket serialization.

#include <array>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>
#include <vector>

#include "endweave/codec/reader.h"
#include "endweave/codec/types/primitives.h"
#include "endweave/codec/writer.h"

namespace endweave {

struct CommandEnum {
    std::string name;
    std::vector<std::int32_t> values;
};

struct CommandSubcommand {
    std::string name;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> values;
};

struct CommandParameter {
    std::string name;
    std::int32_t symbol = 0;
    bool optional = false;
    std::uint8_t options = 0;
};

struct CommandOverload {
    bool chaining = false;
    std::vector<CommandParameter> parameters;
};

struct CommandDefinition {
    std::string name;
    std::string description;
    std::uint16_t flags = 0;
    std::int32_t permission = 0;
    std::int32_t alias_index = 0;
    std::vector<std::int32_t> subcommand_indices;
    std::vector<CommandOverload> overloads;
};

struct CommandConstraint {
    std::int32_t value_index = 0;
    std::int32_t enum_index = 0;
    std::vector<std::uint8_t> constraints;
};

struct SoftEnum {
    std::string name;
    std::vector<std::string> values;
};

struct CommandOrigin {
    std::uint32_t origin_type = 0;
    std::array<std::uint8_t, 16> uuid_bytes = {};
    std::string request_id;
    std::int64_t player_id = -1;
};

// -- Type tags --

struct command_enum_v898 { using value_type = CommandEnum; };
struct command_subcommand_v860 { using value_type = CommandSubcommand; };
struct command_subcommand_v898 { using value_type = CommandSubcommand; };
struct command_definition_v860 { using value_type = CommandDefinition; };
struct command_definition_v898 { using value_type = CommandDefinition; };
struct soft_enum { using value_type = SoftEnum; };
struct command_constraint_t { using value_type = CommandConstraint; };
struct command_origin_v860 { using value_type = CommandOrigin; };
struct command_origin_v898 { using value_type = CommandOrigin; };
struct command_parameter_t { using value_type = CommandParameter; };
struct command_overload_t { using value_type = CommandOverload; };

// -- v860 CommandEnum: variable-width indices based on values_size --

std::expected<CommandEnum, ReadError> read_command_enum_v860(PacketReader& reader, std::uint32_t values_size);
void write_command_enum_v860(PacketWriter& writer, const CommandEnum& value, std::uint32_t values_size);

// -- Reader/writer declarations --

template <> auto PacketReader::read<command_enum_v898>() -> std::expected<CommandEnum, ReadError>;
template <> void PacketWriter::write<command_enum_v898>(const CommandEnum& val);

template <> auto PacketReader::read<command_subcommand_v860>() -> std::expected<CommandSubcommand, ReadError>;
template <> void PacketWriter::write<command_subcommand_v860>(const CommandSubcommand& val);
template <> auto PacketReader::read<command_subcommand_v898>() -> std::expected<CommandSubcommand, ReadError>;
template <> void PacketWriter::write<command_subcommand_v898>(const CommandSubcommand& val);

template <> auto PacketReader::read<command_definition_v860>() -> std::expected<CommandDefinition, ReadError>;
template <> void PacketWriter::write<command_definition_v860>(const CommandDefinition& val);
template <> auto PacketReader::read<command_definition_v898>() -> std::expected<CommandDefinition, ReadError>;
template <> void PacketWriter::write<command_definition_v898>(const CommandDefinition& val);

template <> auto PacketReader::read<soft_enum>() -> std::expected<SoftEnum, ReadError>;
template <> void PacketWriter::write<soft_enum>(const SoftEnum& val);

template <> auto PacketReader::read<command_constraint_t>() -> std::expected<CommandConstraint, ReadError>;
template <> void PacketWriter::write<command_constraint_t>(const CommandConstraint& val);

template <> auto PacketReader::read<command_origin_v860>() -> std::expected<CommandOrigin, ReadError>;
template <> void PacketWriter::write<command_origin_v860>(const CommandOrigin& val);
template <> auto PacketReader::read<command_origin_v898>() -> std::expected<CommandOrigin, ReadError>;
template <> void PacketWriter::write<command_origin_v898>(const CommandOrigin& val);

template <> auto PacketReader::read<command_parameter_t>() -> std::expected<CommandParameter, ReadError>;
template <> void PacketWriter::write<command_parameter_t>(const CommandParameter& val);
template <> auto PacketReader::read<command_overload_t>() -> std::expected<CommandOverload, ReadError>;
template <> void PacketWriter::write<command_overload_t>(const CommandOverload& val);

} // namespace endweave
