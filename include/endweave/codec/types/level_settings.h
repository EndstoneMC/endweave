#pragma once

/// LevelSettings compound type for StartGamePacket.

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

#include "endweave/codec/reader.h"
#include "endweave/codec/types/gameplay.h"
#include "endweave/codec/writer.h"

namespace endweave {

struct LevelSettings {
    std::int64_t seed = 0;
    std::int16_t spawn_biome_type = 0;
    std::string spawn_biome_name;
    std::int32_t spawn_dimension = 0;
    std::int32_t generator = 0;
    std::int32_t game_type = 0;
    bool is_hardcore = false;
    std::int32_t game_difficulty = 0;
    std::int32_t default_spawn_x = 0;
    std::int32_t default_spawn_y = 0;
    std::int32_t default_spawn_z = 0;
    bool achievements_disabled = false;
    std::int32_t editor_world_type = 0;
    bool created_in_editor = false;
    bool exported_from_editor = false;
    std::int32_t day_cycle_stop_time = 0;
    std::int32_t education_edition_offer = 0;
    bool education_features_enabled = false;
    std::string education_product_id;
    float rain_level = 0.0f;
    float lightning_level = 0.0f;
    bool has_confirmed_platform_locked_content = false;
    bool multiplayer_intended = false;
    bool lan_broadcasting_intended = false;
    std::int32_t xbox_live_broadcast_setting = 0;
    std::int32_t platform_broadcast_setting = 0;
    bool commands_enabled = false;
    bool texture_packs_required = false;
    std::vector<GameRule> game_rules;
    std::vector<Experiment> exps;
    bool ever_toggled = false;
    bool has_bonus_chest = false;
    bool start_with_map = false;
    std::int32_t player_permissions = 0;
    std::int32_t server_chunk_tick_range = 0;
    bool has_locked_behavior_pack = false;
    bool has_locked_resource_pack = false;
    bool is_from_locked_template = false;
    bool use_msa_gamertags_only = false;
    bool created_from_template = false;
    bool template_with_locked_settings = false;
    bool only_spawn_v1_villagers = false;
    bool persona_disabled = false;
    bool custom_skins_disabled = false;
    bool emote_chat_muted = false;
    std::string base_game_version;
    std::int32_t limited_world_width = 0;
    std::int32_t limited_world_depth = 0;
    bool nether_type = false;
    std::string edu_shared_uri_button_name;
    std::string edu_shared_uri_link_uri;
    std::optional<bool> override_force_experimental;
    std::uint8_t chat_restriction_level = 0;
    bool disable_player_interactions = false;
};

/// v860: DefaultSpawn Y as uvarint, experiments with int32 count
struct level_settings_v860 { using value_type = LevelSettings; };
/// v924: DefaultSpawn Y as uvarint, experiments with uint32 count
struct level_settings_v924 { using value_type = LevelSettings; };
/// v944: DefaultSpawn Y as varint, experiments with uint32 count
struct level_settings_v944 { using value_type = LevelSettings; };

template <> auto PacketReader::read<level_settings_v860>() -> std::expected<LevelSettings, ReadError>;
template <> auto PacketReader::read<level_settings_v924>() -> std::expected<LevelSettings, ReadError>;
template <> auto PacketReader::read<level_settings_v944>() -> std::expected<LevelSettings, ReadError>;
template <> void PacketWriter::write<level_settings_v860>(const LevelSettings& val);
template <> void PacketWriter::write<level_settings_v924>(const LevelSettings& val);
template <> void PacketWriter::write<level_settings_v944>(const LevelSettings& val);

} // namespace endweave
