"""LevelSettings compound type for StartGamePacket."""

from dataclasses import dataclass

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter

from .gameplay import (
    EXPERIMENTS,
    EXPERIMENTS_V860,
    GAME_RULES,
    Experiment,
    GameRule,
)
from .primitives import (
    BOOL,
    BYTE,
    FLOAT_LE,
    INT64_LE,
    INT_LE,
    SHORT_LE,
    STRING,
    UVAR_INT,
    VAR_INT,
    OptionalType,
    Type,
)

_OPTIONAL_BOOL = OptionalType(BOOL)


@dataclass
class LevelSettings:
    """BDS LevelSettings struct (Seed through DisablePlayerInteractions)."""

    seed: int
    spawn_biome_type: int
    spawn_biome_name: str
    spawn_dimension: int
    generator: int
    game_type: int
    is_hardcore: bool
    game_difficulty: int
    default_spawn_x: int
    default_spawn_y: int
    default_spawn_z: int
    achievements_disabled: bool
    editor_world_type: int
    created_in_editor: bool
    exported_from_editor: bool
    day_cycle_stop_time: int
    education_edition_offer: int
    education_features_enabled: bool
    education_product_id: str
    rain_level: float
    lightning_level: float
    has_confirmed_platform_locked_content: bool
    multiplayer_intended: bool
    lan_broadcasting_intended: bool
    xbox_live_broadcast_setting: int
    platform_broadcast_setting: int
    commands_enabled: bool
    texture_packs_required: bool
    game_rules: list[GameRule]
    experiments: list[Experiment]
    ever_toggled: bool
    has_bonus_chest: bool
    start_with_map: bool
    player_permissions: int
    server_chunk_tick_range: int
    has_locked_behavior_pack: bool
    has_locked_resource_pack: bool
    is_from_locked_template: bool
    use_msa_gamertags_only: bool
    created_from_template: bool
    template_with_locked_settings: bool
    only_spawn_v1_villagers: bool
    persona_disabled: bool
    custom_skins_disabled: bool
    emote_chat_muted: bool
    base_game_version: str
    limited_world_width: int
    limited_world_depth: int
    nether_type: bool
    edu_shared_uri_button_name: str
    edu_shared_uri_link_uri: str
    override_force_experimental: bool | None
    chat_restriction_level: int
    disable_player_interactions: bool


class _LevelSettingsType(Type[LevelSettings]):
    """Version-parameterized LevelSettings reader/writer.

    Args:
        y_type: Type used for DefaultSpawn Y (UVAR_INT or VAR_INT).
        experiments_type: Type used for Experiments list.
    """

    def __init__(
        self,
        y_type: Type[int],
        experiments_type: Type[list[Experiment]],
    ) -> None:
        self._y_type = y_type
        self._experiments_type = experiments_type

    def read(self, reader: PacketReader) -> LevelSettings:
        return LevelSettings(
            seed=INT64_LE.read(reader),
            spawn_biome_type=SHORT_LE.read(reader),
            spawn_biome_name=STRING.read(reader),
            spawn_dimension=VAR_INT.read(reader),
            generator=VAR_INT.read(reader),
            game_type=VAR_INT.read(reader),
            is_hardcore=BOOL.read(reader),
            game_difficulty=VAR_INT.read(reader),
            default_spawn_x=VAR_INT.read(reader),
            default_spawn_y=self._y_type.read(reader),
            default_spawn_z=VAR_INT.read(reader),
            achievements_disabled=BOOL.read(reader),
            editor_world_type=VAR_INT.read(reader),
            created_in_editor=BOOL.read(reader),
            exported_from_editor=BOOL.read(reader),
            day_cycle_stop_time=VAR_INT.read(reader),
            education_edition_offer=VAR_INT.read(reader),
            education_features_enabled=BOOL.read(reader),
            education_product_id=STRING.read(reader),
            rain_level=FLOAT_LE.read(reader),
            lightning_level=FLOAT_LE.read(reader),
            has_confirmed_platform_locked_content=BOOL.read(reader),
            multiplayer_intended=BOOL.read(reader),
            lan_broadcasting_intended=BOOL.read(reader),
            xbox_live_broadcast_setting=VAR_INT.read(reader),
            platform_broadcast_setting=VAR_INT.read(reader),
            commands_enabled=BOOL.read(reader),
            texture_packs_required=BOOL.read(reader),
            game_rules=GAME_RULES.read(reader),
            experiments=self._experiments_type.read(reader),
            ever_toggled=BOOL.read(reader),
            has_bonus_chest=BOOL.read(reader),
            start_with_map=BOOL.read(reader),
            player_permissions=VAR_INT.read(reader),
            server_chunk_tick_range=INT_LE.read(reader),
            has_locked_behavior_pack=BOOL.read(reader),
            has_locked_resource_pack=BOOL.read(reader),
            is_from_locked_template=BOOL.read(reader),
            use_msa_gamertags_only=BOOL.read(reader),
            created_from_template=BOOL.read(reader),
            template_with_locked_settings=BOOL.read(reader),
            only_spawn_v1_villagers=BOOL.read(reader),
            persona_disabled=BOOL.read(reader),
            custom_skins_disabled=BOOL.read(reader),
            emote_chat_muted=BOOL.read(reader),
            base_game_version=STRING.read(reader),
            limited_world_width=INT_LE.read(reader),
            limited_world_depth=INT_LE.read(reader),
            nether_type=BOOL.read(reader),
            edu_shared_uri_button_name=STRING.read(reader),
            edu_shared_uri_link_uri=STRING.read(reader),
            override_force_experimental=_OPTIONAL_BOOL.read(reader),
            chat_restriction_level=BYTE.read(reader),
            disable_player_interactions=BOOL.read(reader),
        )

    def write(self, writer: PacketWriter, value: LevelSettings) -> None:
        INT64_LE.write(writer, value.seed)
        SHORT_LE.write(writer, value.spawn_biome_type)
        STRING.write(writer, value.spawn_biome_name)
        VAR_INT.write(writer, value.spawn_dimension)
        VAR_INT.write(writer, value.generator)
        VAR_INT.write(writer, value.game_type)
        BOOL.write(writer, value.is_hardcore)
        VAR_INT.write(writer, value.game_difficulty)
        VAR_INT.write(writer, value.default_spawn_x)
        self._y_type.write(writer, value.default_spawn_y)
        VAR_INT.write(writer, value.default_spawn_z)
        BOOL.write(writer, value.achievements_disabled)
        VAR_INT.write(writer, value.editor_world_type)
        BOOL.write(writer, value.created_in_editor)
        BOOL.write(writer, value.exported_from_editor)
        VAR_INT.write(writer, value.day_cycle_stop_time)
        VAR_INT.write(writer, value.education_edition_offer)
        BOOL.write(writer, value.education_features_enabled)
        STRING.write(writer, value.education_product_id)
        FLOAT_LE.write(writer, value.rain_level)
        FLOAT_LE.write(writer, value.lightning_level)
        BOOL.write(writer, value.has_confirmed_platform_locked_content)
        BOOL.write(writer, value.multiplayer_intended)
        BOOL.write(writer, value.lan_broadcasting_intended)
        VAR_INT.write(writer, value.xbox_live_broadcast_setting)
        VAR_INT.write(writer, value.platform_broadcast_setting)
        BOOL.write(writer, value.commands_enabled)
        BOOL.write(writer, value.texture_packs_required)
        GAME_RULES.write(writer, value.game_rules)
        self._experiments_type.write(writer, value.experiments)
        BOOL.write(writer, value.ever_toggled)
        BOOL.write(writer, value.has_bonus_chest)
        BOOL.write(writer, value.start_with_map)
        VAR_INT.write(writer, value.player_permissions)
        INT_LE.write(writer, value.server_chunk_tick_range)
        BOOL.write(writer, value.has_locked_behavior_pack)
        BOOL.write(writer, value.has_locked_resource_pack)
        BOOL.write(writer, value.is_from_locked_template)
        BOOL.write(writer, value.use_msa_gamertags_only)
        BOOL.write(writer, value.created_from_template)
        BOOL.write(writer, value.template_with_locked_settings)
        BOOL.write(writer, value.only_spawn_v1_villagers)
        BOOL.write(writer, value.persona_disabled)
        BOOL.write(writer, value.custom_skins_disabled)
        BOOL.write(writer, value.emote_chat_muted)
        STRING.write(writer, value.base_game_version)
        INT_LE.write(writer, value.limited_world_width)
        INT_LE.write(writer, value.limited_world_depth)
        BOOL.write(writer, value.nether_type)
        STRING.write(writer, value.edu_shared_uri_button_name)
        STRING.write(writer, value.edu_shared_uri_link_uri)
        _OPTIONAL_BOOL.write(writer, value.override_force_experimental)
        BYTE.write(writer, value.chat_restriction_level)
        BOOL.write(writer, value.disable_player_interactions)


LEVEL_SETTINGS_V860 = _LevelSettingsType(UVAR_INT, EXPERIMENTS_V860)
LEVEL_SETTINGS_V924 = _LevelSettingsType(UVAR_INT, EXPERIMENTS)
LEVEL_SETTINGS_V944 = _LevelSettingsType(VAR_INT, EXPERIMENTS)
