/// v860 -> v898 packet handlers.

#include "endweave/protocol/v860_to_v898/handlers.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "endweave/codec/types/commands.h"
#include "endweave/codec/types/enums.h"
#include "endweave/codec/types/level_settings.h"
#include "endweave/codec/types/nbt.h"
#include "endweave/codec/types/primitives.h"

namespace endweave::v860_to_v898 {

namespace {

constexpr std::uint8_t ACTOR_EVENT_KINETIC_DAMAGE_DEALT = 80;
constexpr std::int32_t ROWING_FLAG = 0x80;

// -- CommandOutputType label mapping --

const std::unordered_map<int, std::string>& output_type_labels() {
    static const std::unordered_map<int, std::string> map = {
        {0, "none"}, {1, "lastoutput"}, {2, "silent"},
        {3, "alloutput"}, {4, "dataset"},
    };
    return map;
}

// -- TextPacketType label groups --
// enum_to_label strips underscores and lowercases: RAW -> "raw", SYSTEM_MESSAGE -> "systemmessage"

struct TextLabelGroup {
    std::vector<std::string> labels;
};

const TextLabelGroup& message_only_labels() {
    static const TextLabelGroup g = {{
        "raw", "chat", "translate", "popup", "jukeboxpopup", "tip",
    }};
    return g;
}

const TextLabelGroup& author_and_message_labels() {
    static const TextLabelGroup g = {{
        "chat", "whisper", "announcement",
    }};
    return g;
}

const TextLabelGroup& message_and_params_labels() {
    static const TextLabelGroup g = {{
        "translate", "popup", "jukeboxpopup",
    }};
    return g;
}

bool is_message_only(std::uint8_t t) {
    return t == static_cast<std::uint8_t>(TextPacketType::Raw)
        || t == static_cast<std::uint8_t>(TextPacketType::Tip)
        || t == static_cast<std::uint8_t>(TextPacketType::SystemMessage)
        || t == static_cast<std::uint8_t>(TextPacketType::TextObjectWhisper)
        || t == static_cast<std::uint8_t>(TextPacketType::TextObjectAnnouncement)
        || t == static_cast<std::uint8_t>(TextPacketType::TextObject);
}

bool is_author_and_message(std::uint8_t t) {
    return t == static_cast<std::uint8_t>(TextPacketType::Chat)
        || t == static_cast<std::uint8_t>(TextPacketType::Whisper)
        || t == static_cast<std::uint8_t>(TextPacketType::Announcement);
}

bool is_message_and_params(std::uint8_t t) {
    return t == static_cast<std::uint8_t>(TextPacketType::Translate)
        || t == static_cast<std::uint8_t>(TextPacketType::Popup)
        || t == static_cast<std::uint8_t>(TextPacketType::JukeboxPopup);
}

// Build the full label list for a text type group (all labels from the group)
const std::vector<std::string>& get_text_labels(std::uint8_t text_type) {
    if (is_message_only(text_type)) return message_only_labels().labels;
    if (is_author_and_message(text_type)) return author_and_message_labels().labels;
    return message_and_params_labels().labels;
}

} // namespace

// -- actor_event --

void rewrite_actor_event(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uvar_int64>(); // Target Runtime ID
    auto event_id = wrapper.read<byte_t>();
    if (!event_id) return;
    auto remapped = *event_id >= ACTOR_EVENT_KINETIC_DAMAGE_DEALT
        ? static_cast<std::uint8_t>(*event_id + 1) : *event_id;
    wrapper.write<byte_t>(remapped); // Event ID
}

// -- animate --

void rewrite_animate_clientbound(PacketWrapper& wrapper) {
    auto action = wrapper.read<var_int>(); // Action (varint in v860)
    if (!action) return;
    wrapper.write<byte_t>(static_cast<std::uint8_t>(*action)); // Action (uint8 in v898)
    (void)wrapper.passthrough<uvar_int64>(); // Target Runtime ID
    (void)wrapper.passthrough<float_le>(); // Data
    if (*action & ROWING_FLAG) {
        (void)wrapper.read<float_le>(); // Rowing Time (strip for v898)
    }
    wrapper.write<optional_string>(std::nullopt); // Swing Source
}

void rewrite_animate_serverbound(PacketWrapper& wrapper) {
    auto action = wrapper.read<byte_t>(); // Action (uint8 in v898)
    if (!action) return;
    wrapper.write<var_int>(static_cast<std::int32_t>(*action)); // Action (varint in v860)
    (void)wrapper.passthrough<uvar_int64>(); // Target Runtime ID
    (void)wrapper.passthrough<float_le>(); // Data
    (void)wrapper.read<optional_string>(); // Swing Source (strip for v860)
}

// -- camera_aim_assist --

void rewrite_camera_aim_assist_presets(PacketWrapper& wrapper) {
    auto categories_count = wrapper.passthrough<uvar_int>();
    if (!categories_count) return;
    for (std::uint32_t c = 0; c < *categories_count; ++c) {
        (void)wrapper.passthrough<string>(); // Identifier

        auto cat_count = wrapper.passthrough<uvar_int>();
        if (!cat_count) return;
        for (std::uint32_t i = 0; i < *cat_count; ++i) {
            (void)wrapper.passthrough<string>();
            (void)wrapper.passthrough<int_le>();
        }

        auto block_count = wrapper.passthrough<uvar_int>();
        if (!block_count) return;
        for (std::uint32_t i = 0; i < *block_count; ++i) {
            (void)wrapper.passthrough<string>();
            (void)wrapper.passthrough<int_le>();
        }

        wrapper.write<uvar_int>(0); // Entity Tag Priorities (empty for v898)

        (void)wrapper.passthrough<optional_int_le>(); // Default Entity Priority
        (void)wrapper.passthrough<optional_int_le>(); // Default Block Priority
    }

    auto preset_count = wrapper.passthrough<uvar_int>();
    if (!preset_count) return;
    for (std::uint32_t p = 0; p < *preset_count; ++p) {
        (void)wrapper.passthrough<string>(); // Identifier

        auto exclusion_count = wrapper.passthrough<uvar_int>();
        if (!exclusion_count) return;
        for (std::uint32_t i = 0; i < *exclusion_count; ++i) {
            (void)wrapper.passthrough<string>();
        }

        wrapper.write<uvar_int>(0); // Entity Exclusions (empty for v898)
        wrapper.write<uvar_int>(0); // Block Tag Exclusions (empty for v898)

        auto liquid_count = wrapper.passthrough<uvar_int>();
        if (!liquid_count) return;
        for (std::uint32_t i = 0; i < *liquid_count; ++i) {
            (void)wrapper.passthrough<string>();
        }

        auto item_count = wrapper.passthrough<uvar_int>();
        if (!item_count) return;
        for (std::uint32_t i = 0; i < *item_count; ++i) {
            (void)wrapper.passthrough<string>();
            (void)wrapper.passthrough<string>();
        }

        (void)wrapper.passthrough<optional_string>(); // Default Category for Hands
        (void)wrapper.passthrough<optional_string>(); // Default Category for Items
    }
}

// -- commands --

void rewrite_available_commands(PacketWrapper& wrapper) {
    // Enum Values array
    auto enum_values_count = wrapper.passthrough<uvar_int>();
    if (!enum_values_count) return;
    for (std::uint32_t i = 0; i < *enum_values_count; ++i) {
        (void)wrapper.passthrough<string>();
    }
    std::uint32_t values_size = *enum_values_count;

    // Subcommand Values
    auto subcmd_vals_count = wrapper.passthrough<uvar_int>();
    if (!subcmd_vals_count) return;
    for (std::uint32_t i = 0; i < *subcmd_vals_count; ++i) {
        (void)wrapper.passthrough<string>();
    }

    // Postfixes
    auto postfix_count = wrapper.passthrough<uvar_int>();
    if (!postfix_count) return;
    for (std::uint32_t i = 0; i < *postfix_count; ++i) {
        (void)wrapper.passthrough<string>();
    }

    // Enums: v860 (variable width) -> v898 (INT_LE)
    auto enum_count = wrapper.read<uvar_int>();
    if (!enum_count) return;
    std::vector<CommandEnum> enums;
    enums.reserve(*enum_count);
    for (std::uint32_t i = 0; i < *enum_count; ++i) {
        auto e = read_command_enum_v860(wrapper.reader(), values_size);
        if (!e) return;
        enums.push_back(std::move(*e));
    }
    wrapper.write<uvar_int>(static_cast<std::uint32_t>(enums.size()));
    for (auto& e : enums) wrapper.writer().write<command_enum_v898>(e);

    // Subcommands: v860 (USHORT_LE) -> v898 (UVAR_INT)
    auto subcmd_count = wrapper.read<uvar_int>();
    if (!subcmd_count) return;
    std::vector<CommandSubcommand> subcmds;
    subcmds.reserve(*subcmd_count);
    for (std::uint32_t i = 0; i < *subcmd_count; ++i) {
        auto s = wrapper.reader().read<command_subcommand_v860>();
        if (!s) return;
        subcmds.push_back(std::move(*s));
    }
    wrapper.write<uvar_int>(static_cast<std::uint32_t>(subcmds.size()));
    for (auto& s : subcmds) wrapper.writer().write<command_subcommand_v898>(s);

    // Commands: v860 -> v898
    auto cmd_count = wrapper.read<uvar_int>();
    if (!cmd_count) return;
    std::vector<CommandDefinition> cmds;
    cmds.reserve(*cmd_count);
    for (std::uint32_t i = 0; i < *cmd_count; ++i) {
        auto c = wrapper.reader().read<command_definition_v860>();
        if (!c) return;
        cmds.push_back(std::move(*c));
    }
    wrapper.write<uvar_int>(static_cast<std::uint32_t>(cmds.size()));
    for (auto& c : cmds) wrapper.writer().write<command_definition_v898>(c);

    // Soft Enums (passthrough)
    auto soft_count = wrapper.passthrough<uvar_int>();
    if (!soft_count) return;
    for (std::uint32_t i = 0; i < *soft_count; ++i) {
        (void)wrapper.passthrough<soft_enum>();
    }

    // Constraints (passthrough)
    auto constraint_count = wrapper.passthrough<uvar_int>();
    if (!constraint_count) return;
    for (std::uint32_t i = 0; i < *constraint_count; ++i) {
        (void)wrapper.passthrough<command_constraint_t>();
    }
}

void rewrite_command_request(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<string>(); // Command
    auto origin = wrapper.read<command_origin_v898>(); // Origin
    if (!origin) return;
    CommandOrigin v860_origin;
    v860_origin.origin_type = 0; // Player
    v860_origin.uuid_bytes = origin->uuid_bytes;
    v860_origin.request_id = origin->request_id;
    v860_origin.player_id = -1;
    wrapper.write<command_origin_v860>(v860_origin); // Command Origin
    (void)wrapper.passthrough<bool_t>(); // Is Internal Source?
    (void)wrapper.read<string>(); // Version (strip string)
    wrapper.write<var_int>(static_cast<std::int32_t>(CurrentCmdVersion::CloneExtraBlockFilterFix)); // Version
}

void rewrite_command_output(PacketWrapper& wrapper) {
    // Origin: v860 -> v898
    auto origin = wrapper.read<command_origin_v860>();
    if (!origin) return;
    wrapper.write<command_origin_v898>(*origin);

    auto output_type = wrapper.read<byte_t>(); // Output Type (byte in v860)
    if (!output_type) return;
    auto it = output_type_labels().find(static_cast<int>(*output_type));
    wrapper.write<string>(it != output_type_labels().end() ? it->second : "none"); // Output Type (string in v898)

    // Success Count: uvarint -> int_le
    auto success = wrapper.read<uvar_int>();
    if (!success) return;
    wrapper.write<int_le>(static_cast<std::int32_t>(*success));

    auto message_count = wrapper.passthrough<uvar_int>(); // Output Messages
    if (!message_count) return;
    for (std::uint32_t i = 0; i < *message_count; ++i) {
        // v860 order: Successful?, Message ID, Parameters
        // v898 order: Message ID, Successful?, Parameters
        auto internal = wrapper.read<bool_t>();
        auto message_id = wrapper.read<string>();
        auto param_count = wrapper.read<uvar_int>();
        if (!internal || !message_id || !param_count) return;
        std::vector<std::string> parameters;
        parameters.reserve(*param_count);
        for (std::uint32_t j = 0; j < *param_count; ++j) {
            auto p = wrapper.read<string>(); if (!p) return;
            parameters.push_back(std::move(*p));
        }

        wrapper.write<string>(*message_id); // Message ID
        wrapper.write<bool_t>(*internal); // Successful?
        wrapper.write<uvar_int>(static_cast<std::uint32_t>(parameters.size())); // Parameters
        for (auto& p : parameters) wrapper.write<string>(p);
    }

    wrapper.write<bool_t>(*output_type == static_cast<std::uint8_t>(CommandOutputType::DataSet)); // Data Set
}

// -- event --

void rewrite_event(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int64>(); // Target Actor ID
    auto event_type = wrapper.passthrough<var_int>(); // Event Type
    (void)wrapper.passthrough<bool_t>(); // Use Player ID
    if (event_type) {
        wrapper.write<uvar_int>(static_cast<std::uint32_t>(*event_type)); // Event Data (new in v898)
    }
    wrapper.passthrough_all();
}

// -- interact --

void rewrite_interact(PacketWrapper& wrapper) {
    auto action = wrapper.passthrough<byte_t>(); // Action
    (void)wrapper.passthrough<uvar_int64>(); // Target Runtime ID

    auto has_mouse = wrapper.read<bool_t>();
    if (!action || !has_mouse) return;

    bool needs_pos = *action == static_cast<std::uint8_t>(InteractAction::InteractUpdate)
                  || *action == static_cast<std::uint8_t>(InteractAction::StopRiding);

    if (needs_pos) {
        if (*has_mouse) {
            (void)wrapper.passthrough<vec3>(); // Position
        } else {
            wrapper.write<vec3>(Vec3Value{0.0f, 0.0f, 0.0f}); // Position
        }
    } else if (*has_mouse) {
        (void)wrapper.read<vec3>(); // Position (strip)
    }
}

// -- mob_effect --

void rewrite_mob_effect(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uvar_int64>(); // Target Runtime ID
    (void)wrapper.passthrough<byte_t>(); // Event ID
    (void)wrapper.passthrough<var_int>(); // Effect ID
    (void)wrapper.passthrough<var_int>(); // Effect Amplifier
    (void)wrapper.passthrough<bool_t>(); // Show Particles
    (void)wrapper.passthrough<var_int>(); // Effect Duration Ticks
    (void)wrapper.passthrough<uvar_int64>(); // Tick
    wrapper.write<bool_t>(false); // Ambient
}

// -- resource_pack_stack --

void rewrite_resource_pack_stack(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<bool_t>(); // Texture Pack Required

    // Drop behavior pack array (v860 has it, v898 does not)
    auto behavior_count = wrapper.read<uvar_int>();
    if (!behavior_count) return;
    for (std::uint32_t i = 0; i < *behavior_count; ++i) {
        (void)wrapper.read<string>();
        (void)wrapper.read<string>();
        (void)wrapper.read<string>();
    }

    // Passthrough resource pack array
    auto resource_count = wrapper.passthrough<uvar_int>();
    if (!resource_count) return;
    for (std::uint32_t i = 0; i < *resource_count; ++i) {
        (void)wrapper.passthrough<string>();
        (void)wrapper.passthrough<string>();
        (void)wrapper.passthrough<string>();
    }
}

// -- start_game --

void rewrite_start_game(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int64>(); // Entity ID
    (void)wrapper.passthrough<uvar_int64>(); // Runtime ID
    (void)wrapper.passthrough<var_int>(); // Game Type
    (void)wrapper.passthrough<vec3>(); // Position
    (void)wrapper.passthrough<vec2>(); // Rotation

    (void)wrapper.passthrough<level_settings_v860>();

    // Server Telemetry
    (void)wrapper.passthrough<string>(); // Server ID
    (void)wrapper.passthrough<string>(); // World ID
    (void)wrapper.passthrough<string>(); // Scenario ID
    (void)wrapper.passthrough<string>(); // Owner ID

    // Post-LevelSettings fields
    (void)wrapper.passthrough<string>(); // Level ID
    (void)wrapper.passthrough<string>(); // Level Name
    (void)wrapper.passthrough<string>(); // Template Content Identity
    (void)wrapper.passthrough<bool_t>(); // Is Trial?
    (void)wrapper.passthrough<var_int>(); // Movement Settings.RewindHistorySize
    (void)wrapper.passthrough<bool_t>(); // Movement Settings.ServerAuthBlockBreaking
    (void)wrapper.passthrough<int64_le>(); // Level Current Time
    (void)wrapper.passthrough<var_int>(); // Enchantment Seed

    auto block_prop_count = wrapper.passthrough<uvar_int>(); // Block Properties
    if (block_prop_count) {
        for (std::uint32_t i = 0; i < *block_prop_count; ++i) {
            (void)wrapper.passthrough<string>();
            (void)wrapper.passthrough<named_compound_tag>();
        }
    }

    (void)wrapper.passthrough<string>(); // Multiplayer Correlation Id
    (void)wrapper.passthrough<bool_t>(); // Enable Item Stack Net Manager
    (void)wrapper.passthrough<string>(); // Server version
    (void)wrapper.passthrough<named_compound_tag>(); // Player Property Data
    (void)wrapper.read<int64_le>(); // Server Block Type Registry Checksum
    wrapper.write<int64_le>(0); // zero checksum to skip validation
    (void)wrapper.passthrough<int64_le>(); // World Template ID (MSB)
    (void)wrapper.passthrough<int64_le>(); // World Template ID (LSB)
    (void)wrapper.passthrough<bool_t>(); // Server Enabled ClientSide Generation
    (void)wrapper.passthrough<bool_t>(); // BlockNetworkIds Are Hashes
    (void)wrapper.read<bool_t>(); // TickDeathSystemsEnabled (dropped in v898)
}

// -- text --

void rewrite_text_clientbound(PacketWrapper& wrapper) {
    auto text_type = wrapper.read<byte_t>();
    auto needs_translation = wrapper.read<bool_t>();
    if (!text_type || !needs_translation) return;

    wrapper.write<bool_t>(*needs_translation);

    if (is_message_only(*text_type)) {
        wrapper.write<byte_t>(static_cast<std::uint8_t>(TextPacketBodyType::MessageOnly));
        for (auto& label : message_only_labels().labels) wrapper.write<string>(label);
        wrapper.write<byte_t>(*text_type);
        (void)wrapper.passthrough<string>(); // Message
    } else if (is_author_and_message(*text_type)) {
        wrapper.write<byte_t>(static_cast<std::uint8_t>(TextPacketBodyType::AuthorAndMessage));
        for (auto& label : author_and_message_labels().labels) wrapper.write<string>(label);
        wrapper.write<byte_t>(*text_type);
        (void)wrapper.passthrough<string>(); // Source Name
        (void)wrapper.passthrough<string>(); // Message
    } else if (is_message_and_params(*text_type)) {
        wrapper.write<byte_t>(static_cast<std::uint8_t>(TextPacketBodyType::MessageAndParams));
        for (auto& label : message_and_params_labels().labels) wrapper.write<string>(label);
        wrapper.write<byte_t>(*text_type);
        (void)wrapper.passthrough<string>(); // Message
        auto param_count = wrapper.passthrough<uvar_int>();
        if (param_count) {
            for (std::uint32_t i = 0; i < *param_count; ++i) {
                (void)wrapper.passthrough<string>();
            }
        }
    } else {
        return; // Unknown text type
    }

    (void)wrapper.passthrough<string>(); // XUID
    (void)wrapper.passthrough<string>(); // Platform Chat ID
    auto filtered = wrapper.read<string>(); // Filtered Message
    if (filtered && !filtered->empty()) {
        wrapper.write<optional_string>(*filtered);
    } else {
        wrapper.write<optional_string>(std::nullopt);
    }
}

void rewrite_text_serverbound(PacketWrapper& wrapper) {
    auto needs_translation = wrapper.read<bool_t>();
    auto kind = wrapper.read<byte_t>();
    if (!needs_translation || !kind) return;

    if (*kind == static_cast<std::uint8_t>(TextPacketBodyType::MessageOnly)) {
        for (int i = 0; i < 6; ++i) (void)wrapper.read<string>(); // labels
        auto text_type = wrapper.read<byte_t>();
        if (!text_type) return;
        wrapper.write<byte_t>(*text_type);
        wrapper.write<bool_t>(*needs_translation);
        (void)wrapper.passthrough<string>(); // Message
    } else if (*kind == static_cast<std::uint8_t>(TextPacketBodyType::AuthorAndMessage)) {
        for (int i = 0; i < 3; ++i) (void)wrapper.read<string>(); // labels
        auto text_type = wrapper.read<byte_t>();
        if (!text_type) return;
        wrapper.write<byte_t>(*text_type);
        wrapper.write<bool_t>(*needs_translation);
        (void)wrapper.passthrough<string>(); // Source Name
        (void)wrapper.passthrough<string>(); // Message
    } else if (*kind == static_cast<std::uint8_t>(TextPacketBodyType::MessageAndParams)) {
        for (int i = 0; i < 3; ++i) (void)wrapper.read<string>(); // labels
        auto text_type = wrapper.read<byte_t>();
        if (!text_type) return;
        wrapper.write<byte_t>(*text_type);
        wrapper.write<bool_t>(*needs_translation);
        (void)wrapper.passthrough<string>(); // Message
        auto param_count = wrapper.passthrough<uvar_int>();
        if (param_count) {
            for (std::uint32_t i = 0; i < *param_count; ++i) {
                (void)wrapper.passthrough<string>();
            }
        }
    } else {
        return; // Unknown kind
    }

    (void)wrapper.passthrough<string>(); // XUID
    (void)wrapper.passthrough<string>(); // Platform Chat ID
    auto filtered = wrapper.read<optional_string>();
    wrapper.write<string>(filtered && filtered->has_value() ? **filtered : std::string(""));
}

} // namespace endweave::v860_to_v898
