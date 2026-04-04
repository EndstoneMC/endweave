/// Command compound type codec implementation.

#include "endweave/codec/types/commands.h"

#include <string>
#include <unordered_map>

namespace endweave {

namespace {

// -- Permission label mappings (v898 string-serialized enums) --

const std::unordered_map<std::string, std::int32_t>& permission_by_label() {
    static const std::unordered_map<std::string, std::int32_t> map = {
        {"any", 0}, {"gamedirectors", 1}, {"admin", 2},
        {"host", 3}, {"owner", 4}, {"internal", 5},
    };
    return map;
}

const std::unordered_map<std::int32_t, std::string>& permission_by_value() {
    static const std::unordered_map<std::int32_t, std::string> map = {
        {0, "any"}, {1, "gamedirectors"}, {2, "admin"},
        {3, "host"}, {4, "owner"}, {5, "internal"},
    };
    return map;
}

// -- CommandOriginType label mappings --

const std::unordered_map<std::string, std::uint32_t>& origin_type_by_label() {
    static const std::unordered_map<std::string, std::uint32_t> map = {
        {"player", 0}, {"commandblock", 1}, {"minecartcommandblock", 2},
        {"devconsole", 3}, {"test", 4}, {"automationplayer", 5},
        {"clientautomation", 6}, {"dedicatedserver", 7}, {"entity", 8},
        {"virtual", 9}, {"gameargument", 10}, {"entityserver", 11},
        {"precompiled", 12}, {"gamedirectorentityserver", 13},
        {"scripting", 14}, {"executecontext", 15},
    };
    return map;
}

const std::unordered_map<std::uint32_t, std::string>& origin_type_by_value() {
    static const std::unordered_map<std::uint32_t, std::string> map = {
        {0, "player"}, {1, "commandblock"}, {2, "minecartcommandblock"},
        {3, "devconsole"}, {4, "test"}, {5, "automationplayer"},
        {6, "clientautomation"}, {7, "dedicatedserver"}, {8, "entity"},
        {9, "virtual"}, {10, "gameargument"}, {11, "entityserver"},
        {12, "precompiled"}, {13, "gamedirectorentityserver"},
        {14, "scripting"}, {15, "executecontext"},
    };
    return map;
}

// CommandOriginType values that include player_id in v860 format
constexpr std::uint32_t ORIGIN_TEST = 4;
constexpr std::uint32_t ORIGIN_AUTOMATION_PLAYER = 5;

} // namespace

// -- CommandParameter --

template <> auto PacketReader::read<command_parameter_t>() -> std::expected<CommandParameter, ReadError> {
    CommandParameter p;
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); p.name = std::move(*v); }
    { auto v = read_int_le(); if (!v) return std::unexpected(v.error()); p.symbol = *v; }
    { auto v = read_bool(); if (!v) return std::unexpected(v.error()); p.optional = *v; }
    { auto v = read_byte(); if (!v) return std::unexpected(v.error()); p.options = *v; }
    return p;
}

template <> void PacketWriter::write<command_parameter_t>(const CommandParameter& val) {
    write_string(val.name);
    write_int_le(val.symbol);
    write_bool(val.optional);
    write_byte(val.options);
}

// -- CommandOverload --

template <> auto PacketReader::read<command_overload_t>() -> std::expected<CommandOverload, ReadError> {
    CommandOverload o;
    { auto v = read_bool(); if (!v) return std::unexpected(v.error()); o.chaining = *v; }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      o.parameters.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto p = read<command_parameter_t>();
          if (!p) return std::unexpected(p.error());
          o.parameters.push_back(std::move(*p));
      }
    }
    return o;
}

template <> void PacketWriter::write<command_overload_t>(const CommandOverload& val) {
    write_bool(val.chaining);
    write_uvarint(static_cast<std::uint32_t>(val.parameters.size()));
    for (auto& p : val.parameters) write<command_parameter_t>(p);
}

// -- CommandEnum v898 (always INT_LE indices) --

template <> auto PacketReader::read<command_enum_v898>() -> std::expected<CommandEnum, ReadError> {
    CommandEnum e;
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); e.name = std::move(*v); }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      e.values.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read_int_le(); if (!v) return std::unexpected(v.error());
          e.values.push_back(*v);
      }
    }
    return e;
}

template <> void PacketWriter::write<command_enum_v898>(const CommandEnum& val) {
    write_string(val.name);
    write_uvarint(static_cast<std::uint32_t>(val.values.size()));
    for (auto v : val.values) write_int_le(v);
}

// -- CommandEnum v860 (variable-width indices based on values_size) --

std::expected<CommandEnum, ReadError> read_command_enum_v860(PacketReader& reader, std::uint32_t values_size) {
    CommandEnum e;
    { auto v = reader.read_string(); if (!v) return std::unexpected(v.error()); e.name = std::move(*v); }
    { auto count = reader.read_uvarint(); if (!count) return std::unexpected(count.error());
      e.values.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          if (values_size <= 0x100) {
              auto v = reader.read_byte(); if (!v) return std::unexpected(v.error());
              e.values.push_back(static_cast<std::int32_t>(*v));
          } else if (values_size <= 0x10000) {
              auto v = reader.read_ushort_le(); if (!v) return std::unexpected(v.error());
              e.values.push_back(static_cast<std::int32_t>(*v));
          } else {
              auto v = reader.read_int_le(); if (!v) return std::unexpected(v.error());
              e.values.push_back(*v);
          }
      }
    }
    return e;
}

void write_command_enum_v860(PacketWriter& writer, const CommandEnum& val, std::uint32_t values_size) {
    writer.write_string(val.name);
    writer.write_uvarint(static_cast<std::uint32_t>(val.values.size()));
    for (auto v : val.values) {
        if (values_size <= 0x100) {
            writer.write_byte(static_cast<std::uint8_t>(v));
        } else if (values_size <= 0x10000) {
            writer.write_ushort_le(static_cast<std::uint16_t>(v));
        } else {
            writer.write_int_le(v);
        }
    }
}

// -- CommandSubcommand v860 (USHORT_LE pairs) --

template <> auto PacketReader::read<command_subcommand_v860>() -> std::expected<CommandSubcommand, ReadError> {
    CommandSubcommand s;
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); s.name = std::move(*v); }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      s.values.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto a = read_ushort_le(); if (!a) return std::unexpected(a.error());
          auto b = read_ushort_le(); if (!b) return std::unexpected(b.error());
          s.values.emplace_back(*a, *b);
      }
    }
    return s;
}

template <> void PacketWriter::write<command_subcommand_v860>(const CommandSubcommand& val) {
    write_string(val.name);
    write_uvarint(static_cast<std::uint32_t>(val.values.size()));
    for (auto& [a, b] : val.values) {
        write_ushort_le(static_cast<std::uint16_t>(a));
        write_ushort_le(static_cast<std::uint16_t>(b));
    }
}

// -- CommandSubcommand v898 (UVAR_INT pairs) --

template <> auto PacketReader::read<command_subcommand_v898>() -> std::expected<CommandSubcommand, ReadError> {
    CommandSubcommand s;
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); s.name = std::move(*v); }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      s.values.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto a = read_uvarint(); if (!a) return std::unexpected(a.error());
          auto b = read_uvarint(); if (!b) return std::unexpected(b.error());
          s.values.emplace_back(*a, *b);
      }
    }
    return s;
}

template <> void PacketWriter::write<command_subcommand_v898>(const CommandSubcommand& val) {
    write_string(val.name);
    write_uvarint(static_cast<std::uint32_t>(val.values.size()));
    for (auto& [a, b] : val.values) {
        write_uvarint(a);
        write_uvarint(b);
    }
}

// -- CommandDefinition v860 (BYTE permission, USHORT_LE subcommand indices) --

template <> auto PacketReader::read<command_definition_v860>() -> std::expected<CommandDefinition, ReadError> {
    CommandDefinition d;
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); d.name = std::move(*v); }
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); d.description = std::move(*v); }
    { auto v = read_ushort_le(); if (!v) return std::unexpected(v.error()); d.flags = *v; }
    { auto v = read_byte(); if (!v) return std::unexpected(v.error()); d.permission = static_cast<std::int32_t>(*v); }
    { auto v = read_int_le(); if (!v) return std::unexpected(v.error()); d.alias_index = *v; }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      d.subcommand_indices.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read_ushort_le(); if (!v) return std::unexpected(v.error());
          d.subcommand_indices.push_back(static_cast<std::int32_t>(*v));
      }
    }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      d.overloads.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read<command_overload_t>(); if (!v) return std::unexpected(v.error());
          d.overloads.push_back(std::move(*v));
      }
    }
    return d;
}

template <> void PacketWriter::write<command_definition_v860>(const CommandDefinition& val) {
    write_string(val.name);
    write_string(val.description);
    write_ushort_le(val.flags);
    write_byte(static_cast<std::uint8_t>(val.permission));
    write_int_le(val.alias_index);
    write_uvarint(static_cast<std::uint32_t>(val.subcommand_indices.size()));
    for (auto idx : val.subcommand_indices) write_ushort_le(static_cast<std::uint16_t>(idx));
    write_uvarint(static_cast<std::uint32_t>(val.overloads.size()));
    for (auto& o : val.overloads) write<command_overload_t>(o);
}

// -- CommandDefinition v898 (STRING permission label, INT_LE subcommand indices) --

template <> auto PacketReader::read<command_definition_v898>() -> std::expected<CommandDefinition, ReadError> {
    CommandDefinition d;
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); d.name = std::move(*v); }
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); d.description = std::move(*v); }
    { auto v = read_ushort_le(); if (!v) return std::unexpected(v.error()); d.flags = *v; }
    { auto label = read_string(); if (!label) return std::unexpected(label.error());
      auto it = permission_by_label().find(*label);
      d.permission = (it != permission_by_label().end()) ? it->second : 0;
    }
    { auto v = read_int_le(); if (!v) return std::unexpected(v.error()); d.alias_index = *v; }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      d.subcommand_indices.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read_int_le(); if (!v) return std::unexpected(v.error());
          d.subcommand_indices.push_back(*v);
      }
    }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      d.overloads.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read<command_overload_t>(); if (!v) return std::unexpected(v.error());
          d.overloads.push_back(std::move(*v));
      }
    }
    return d;
}

template <> void PacketWriter::write<command_definition_v898>(const CommandDefinition& val) {
    write_string(val.name);
    write_string(val.description);
    write_ushort_le(val.flags);
    auto it = permission_by_value().find(val.permission);
    write_string(it != permission_by_value().end() ? it->second : "any");
    write_int_le(val.alias_index);
    write_uvarint(static_cast<std::uint32_t>(val.subcommand_indices.size()));
    for (auto idx : val.subcommand_indices) write_int_le(idx);
    write_uvarint(static_cast<std::uint32_t>(val.overloads.size()));
    for (auto& o : val.overloads) write<command_overload_t>(o);
}

// -- SoftEnum --

template <> auto PacketReader::read<soft_enum>() -> std::expected<SoftEnum, ReadError> {
    SoftEnum e;
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); e.name = std::move(*v); }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      e.values.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read_string(); if (!v) return std::unexpected(v.error());
          e.values.push_back(std::move(*v));
      }
    }
    return e;
}

template <> void PacketWriter::write<soft_enum>(const SoftEnum& val) {
    write_string(val.name);
    write_uvarint(static_cast<std::uint32_t>(val.values.size()));
    for (auto& v : val.values) write_string(v);
}

// -- CommandConstraint --

template <> auto PacketReader::read<command_constraint_t>() -> std::expected<CommandConstraint, ReadError> {
    CommandConstraint c;
    { auto v = read_int_le(); if (!v) return std::unexpected(v.error()); c.value_index = *v; }
    { auto v = read_int_le(); if (!v) return std::unexpected(v.error()); c.enum_index = *v; }
    { auto count = read_uvarint(); if (!count) return std::unexpected(count.error());
      c.constraints.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read_byte(); if (!v) return std::unexpected(v.error());
          c.constraints.push_back(*v);
      }
    }
    return c;
}

template <> void PacketWriter::write<command_constraint_t>(const CommandConstraint& val) {
    write_int_le(val.value_index);
    write_int_le(val.enum_index);
    write_uvarint(static_cast<std::uint32_t>(val.constraints.size()));
    for (auto c : val.constraints) write_byte(c);
}

// -- CommandOrigin v860 (UVAR_INT type, conditional player_id) --

template <> auto PacketReader::read<command_origin_v860>() -> std::expected<CommandOrigin, ReadError> {
    CommandOrigin o;
    { auto v = read_uvarint(); if (!v) return std::unexpected(v.error()); o.origin_type = *v; }
    { auto v = read<uuid>(); if (!v) return std::unexpected(v.error()); o.uuid_bytes = *v; }
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); o.request_id = std::move(*v); }
    o.player_id = -1;
    if (o.origin_type == ORIGIN_TEST || o.origin_type == ORIGIN_AUTOMATION_PLAYER) {
        auto v = read_varint64(); if (!v) return std::unexpected(v.error());
        o.player_id = *v;
    }
    return o;
}

template <> void PacketWriter::write<command_origin_v860>(const CommandOrigin& val) {
    write_uvarint(val.origin_type);
    write<uuid>(val.uuid_bytes);
    write_string(val.request_id);
    if (val.origin_type == ORIGIN_TEST || val.origin_type == ORIGIN_AUTOMATION_PLAYER) {
        write_varint64(val.player_id);
    }
}

// -- CommandOrigin v898 (STRING type label, always INT64_LE player_id) --

template <> auto PacketReader::read<command_origin_v898>() -> std::expected<CommandOrigin, ReadError> {
    CommandOrigin o;
    { auto label = read_string(); if (!label) return std::unexpected(label.error());
      auto it = origin_type_by_label().find(*label);
      o.origin_type = (it != origin_type_by_label().end()) ? it->second : 0;
    }
    { auto v = read<uuid>(); if (!v) return std::unexpected(v.error()); o.uuid_bytes = *v; }
    { auto v = read_string(); if (!v) return std::unexpected(v.error()); o.request_id = std::move(*v); }
    { auto v = read_int64_le(); if (!v) return std::unexpected(v.error()); o.player_id = *v; }
    return o;
}

template <> void PacketWriter::write<command_origin_v898>(const CommandOrigin& val) {
    auto it = origin_type_by_value().find(val.origin_type);
    write_string(it != origin_type_by_value().end() ? it->second : "player");
    write<uuid>(val.uuid_bytes);
    write_string(val.request_id);
    write_int64_le(val.player_id);
}

} // namespace endweave
