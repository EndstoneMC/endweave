/// LevelSettings codec implementation.

#include "endweave/codec/types/level_settings.h"
#include "endweave/codec/types/primitives.h"

namespace endweave {

namespace {

/// Read spawn Y as either uvarint (v860/v924) or varint (v944).
template <bool UseVarint>
std::expected<LevelSettings, ReadError> read_level_settings(PacketReader& r,
    std::expected<std::vector<Experiment>, ReadError>(*read_exps)(PacketReader&))
{
    LevelSettings s;
    #define TRY(field, expr) { auto _v = (expr); if (!_v) return std::unexpected(_v.error()); s.field = *_v; }

    TRY(seed, r.read_int64_le())
    TRY(spawn_biome_type, r.read_short_le())
    TRY(spawn_biome_name, r.read_string())
    TRY(spawn_dimension, r.read_varint())
    TRY(generator, r.read_varint())
    TRY(game_type, r.read_varint())
    TRY(is_hardcore, r.read_bool())
    TRY(game_difficulty, r.read_varint())
    TRY(default_spawn_x, r.read_varint())
    if constexpr (UseVarint) {
        TRY(default_spawn_y, r.read_varint())
    } else {
        auto y = r.read_uvarint();
        if (!y) return std::unexpected(y.error());
        s.default_spawn_y = static_cast<std::int32_t>(*y);
    }
    TRY(default_spawn_z, r.read_varint())
    TRY(achievements_disabled, r.read_bool())
    TRY(editor_world_type, r.read_varint())
    TRY(created_in_editor, r.read_bool())
    TRY(exported_from_editor, r.read_bool())
    TRY(day_cycle_stop_time, r.read_varint())
    TRY(education_edition_offer, r.read_varint())
    TRY(education_features_enabled, r.read_bool())
    TRY(education_product_id, r.read_string())
    TRY(rain_level, r.read_float_le())
    TRY(lightning_level, r.read_float_le())
    TRY(has_confirmed_platform_locked_content, r.read_bool())
    TRY(multiplayer_intended, r.read_bool())
    TRY(lan_broadcasting_intended, r.read_bool())
    TRY(xbox_live_broadcast_setting, r.read_varint())
    TRY(platform_broadcast_setting, r.read_varint())
    TRY(commands_enabled, r.read_bool())
    TRY(texture_packs_required, r.read_bool())

    { auto v = r.read<game_rules>(); if (!v) return std::unexpected(v.error()); s.game_rules = std::move(*v); }
    { auto v = read_exps(r); if (!v) return std::unexpected(v.error()); s.exps = std::move(*v); }

    TRY(ever_toggled, r.read_bool())
    TRY(has_bonus_chest, r.read_bool())
    TRY(start_with_map, r.read_bool())
    TRY(player_permissions, r.read_varint())
    TRY(server_chunk_tick_range, r.read_int_le())
    TRY(has_locked_behavior_pack, r.read_bool())
    TRY(has_locked_resource_pack, r.read_bool())
    TRY(is_from_locked_template, r.read_bool())
    TRY(use_msa_gamertags_only, r.read_bool())
    TRY(created_from_template, r.read_bool())
    TRY(template_with_locked_settings, r.read_bool())
    TRY(only_spawn_v1_villagers, r.read_bool())
    TRY(persona_disabled, r.read_bool())
    TRY(custom_skins_disabled, r.read_bool())
    TRY(emote_chat_muted, r.read_bool())
    TRY(base_game_version, r.read_string())
    TRY(limited_world_width, r.read_int_le())
    TRY(limited_world_depth, r.read_int_le())
    TRY(nether_type, r.read_bool())
    TRY(edu_shared_uri_button_name, r.read_string())
    TRY(edu_shared_uri_link_uri, r.read_string())

    // Optional<bool>
    {
        auto has = r.read_bool();
        if (!has) return std::unexpected(has.error());
        if (*has) {
            auto v = r.read_bool();
            if (!v) return std::unexpected(v.error());
            s.override_force_experimental = *v;
        }
    }

    TRY(chat_restriction_level, r.read_byte())
    TRY(disable_player_interactions, r.read_bool())

    #undef TRY
    return s;
}

template <bool UseVarint>
void write_level_settings(PacketWriter& w, const LevelSettings& s,
    void(*write_exps)(PacketWriter&, const std::vector<Experiment>&))
{
    w.write_int64_le(s.seed);
    w.write_short_le(s.spawn_biome_type);
    w.write_string(s.spawn_biome_name);
    w.write_varint(s.spawn_dimension);
    w.write_varint(s.generator);
    w.write_varint(s.game_type);
    w.write_bool(s.is_hardcore);
    w.write_varint(s.game_difficulty);
    w.write_varint(s.default_spawn_x);
    if constexpr (UseVarint)
        w.write_varint(s.default_spawn_y);
    else
        w.write_uvarint(static_cast<std::uint32_t>(s.default_spawn_y));
    w.write_varint(s.default_spawn_z);
    w.write_bool(s.achievements_disabled);
    w.write_varint(s.editor_world_type);
    w.write_bool(s.created_in_editor);
    w.write_bool(s.exported_from_editor);
    w.write_varint(s.day_cycle_stop_time);
    w.write_varint(s.education_edition_offer);
    w.write_bool(s.education_features_enabled);
    w.write_string(s.education_product_id);
    w.write_float_le(s.rain_level);
    w.write_float_le(s.lightning_level);
    w.write_bool(s.has_confirmed_platform_locked_content);
    w.write_bool(s.multiplayer_intended);
    w.write_bool(s.lan_broadcasting_intended);
    w.write_varint(s.xbox_live_broadcast_setting);
    w.write_varint(s.platform_broadcast_setting);
    w.write_bool(s.commands_enabled);
    w.write_bool(s.texture_packs_required);
    w.write<game_rules>(s.game_rules);
    write_exps(w, s.exps);
    w.write_bool(s.ever_toggled);
    w.write_bool(s.has_bonus_chest);
    w.write_bool(s.start_with_map);
    w.write_varint(s.player_permissions);
    w.write_int_le(s.server_chunk_tick_range);
    w.write_bool(s.has_locked_behavior_pack);
    w.write_bool(s.has_locked_resource_pack);
    w.write_bool(s.is_from_locked_template);
    w.write_bool(s.use_msa_gamertags_only);
    w.write_bool(s.created_from_template);
    w.write_bool(s.template_with_locked_settings);
    w.write_bool(s.only_spawn_v1_villagers);
    w.write_bool(s.persona_disabled);
    w.write_bool(s.custom_skins_disabled);
    w.write_bool(s.emote_chat_muted);
    w.write_string(s.base_game_version);
    w.write_int_le(s.limited_world_width);
    w.write_int_le(s.limited_world_depth);
    w.write_bool(s.nether_type);
    w.write_string(s.edu_shared_uri_button_name);
    w.write_string(s.edu_shared_uri_link_uri);
    // Optional<bool>
    if (s.override_force_experimental.has_value()) {
        w.write_bool(true);
        w.write_bool(*s.override_force_experimental);
    } else {
        w.write_bool(false);
    }
    w.write_byte(s.chat_restriction_level);
    w.write_bool(s.disable_player_interactions);
}

std::expected<std::vector<Experiment>, ReadError> read_exps_v860(PacketReader& r) { return r.read<experiments_v860>(); }
std::expected<std::vector<Experiment>, ReadError> read_exps_v924(PacketReader& r) { return r.read<experiments>(); }
void write_exps_v860(PacketWriter& w, const std::vector<Experiment>& v) { w.write<experiments_v860>(v); }
void write_exps_v924(PacketWriter& w, const std::vector<Experiment>& v) { w.write<experiments>(v); }

} // namespace

template <> auto PacketReader::read<level_settings_v860>() -> std::expected<LevelSettings, ReadError> {
    return read_level_settings<false>(*this, read_exps_v860);
}
template <> auto PacketReader::read<level_settings_v924>() -> std::expected<LevelSettings, ReadError> {
    return read_level_settings<false>(*this, read_exps_v924);
}
template <> auto PacketReader::read<level_settings_v944>() -> std::expected<LevelSettings, ReadError> {
    return read_level_settings<true>(*this, read_exps_v924);
}
template <> void PacketWriter::write<level_settings_v860>(const LevelSettings& val) {
    write_level_settings<false>(*this, val, write_exps_v860);
}
template <> void PacketWriter::write<level_settings_v924>(const LevelSettings& val) {
    write_level_settings<false>(*this, val, write_exps_v924);
}
template <> void PacketWriter::write<level_settings_v944>(const LevelSettings& val) {
    write_level_settings<true>(*this, val, write_exps_v924);
}

} // namespace endweave
