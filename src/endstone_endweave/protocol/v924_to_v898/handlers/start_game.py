"""Handler for StartGamePacket -- v924 server to v898 client."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    EXPERIMENTS,
    FLOAT_LE,
    INT64_LE,
    INT_LE,
    NAMED_COMPOUND_TAG,
    SHORT_LE,
    STRING,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    CompoundTag,
    GameRuleType,
    OptionalType,
    PacketWrapper,
)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Strip the v924 StartGame additions that v898 does not understand.

    Args:
        wrapper: Packet wrapper for StartGame.
    """
    wrapper.passthrough(VAR_INT64)  # Entity ID
    wrapper.passthrough(UVAR_INT64)  # Runtime ID
    wrapper.passthrough(VAR_INT)  # Game Type
    wrapper.passthrough(FLOAT_LE)  # Position X
    wrapper.passthrough(FLOAT_LE)  # Position Y
    wrapper.passthrough(FLOAT_LE)  # Position Z
    wrapper.passthrough(FLOAT_LE)  # Rotation X
    wrapper.passthrough(FLOAT_LE)  # Rotation Y

    wrapper.passthrough(INT64_LE)  # Settings.Seed
    wrapper.passthrough(SHORT_LE)  # Settings.SpawnSettings.BiomeType
    wrapper.passthrough(STRING)  # Settings.SpawnSettings.UserDefinedBiomeName
    wrapper.passthrough(VAR_INT)  # Settings.SpawnSettings.Dimension
    wrapper.passthrough(VAR_INT)  # Settings.Generator
    wrapper.passthrough(VAR_INT)  # Settings.GameType
    wrapper.passthrough(BOOL)  # Settings.IsHardcore
    wrapper.passthrough(VAR_INT)  # Settings.GameDifficulty
    wrapper.passthrough(VAR_INT)  # Settings.DefaultSpawn X
    wrapper.passthrough(VAR_INT)  # Settings.DefaultSpawn Y
    wrapper.passthrough(VAR_INT)  # Settings.DefaultSpawn Z

    wrapper.passthrough(BOOL)  # Achievements Disabled
    wrapper.passthrough(VAR_INT)  # Editor World Type
    wrapper.passthrough(BOOL)  # Created In Editor
    wrapper.passthrough(BOOL)  # Exported From Editor
    wrapper.passthrough(VAR_INT)  # Day Cycle Stop Time
    wrapper.passthrough(VAR_INT)  # Education Edition Offer
    wrapper.passthrough(BOOL)  # Education features enabled
    wrapper.passthrough(STRING)  # Education product id
    wrapper.passthrough(FLOAT_LE)  # Rain Level
    wrapper.passthrough(FLOAT_LE)  # Lightning Level
    wrapper.passthrough(BOOL)  # Has confirmed Platform Locked Content
    wrapper.passthrough(BOOL)  # Multiplayer intended
    wrapper.passthrough(BOOL)  # LAN broadcasting intended
    wrapper.passthrough(VAR_INT)  # Xbox Live Broadcast Setting
    wrapper.passthrough(VAR_INT)  # Platform Broadcast Setting
    wrapper.passthrough(BOOL)  # Commands Enabled
    wrapper.passthrough(BOOL)  # Texture Packs Required

    game_rule_count = wrapper.passthrough(UVAR_INT)  # GameRules
    for _ in range(game_rule_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(BOOL)
        game_rule_type = wrapper.passthrough(UVAR_INT)
        if game_rule_type == GameRuleType.BOOL:
            wrapper.passthrough(BOOL)
        elif game_rule_type == GameRuleType.INT:
            wrapper.passthrough(VAR_INT)
        elif game_rule_type == GameRuleType.FLOAT:
            wrapper.passthrough(FLOAT_LE)
        else:
            raise ValueError(f"Unknown game rule type: {game_rule_type}")

    wrapper.passthrough(EXPERIMENTS)  # Experiments
    wrapper.passthrough(BOOL)  # ever_toggled

    wrapper.passthrough(BOOL)  # Has Bonus Chest
    wrapper.passthrough(BOOL)  # Start with Map
    wrapper.passthrough(VAR_INT)  # Player Permissions
    wrapper.passthrough(INT_LE)  # Server Chunk Tick Range
    wrapper.passthrough(BOOL)  # Has locked behavior pack?
    wrapper.passthrough(BOOL)  # Has locked resource pack?
    wrapper.passthrough(BOOL)  # Is from locked template?
    wrapper.passthrough(BOOL)  # Use Msa Gamertags Only?
    wrapper.passthrough(BOOL)  # If this world was created from a template.
    wrapper.passthrough(BOOL)  # If this world is a template with locked settings.
    wrapper.passthrough(BOOL)  # Only spawn v1 villagers
    wrapper.passthrough(BOOL)  # PersonaDisabled?
    wrapper.passthrough(BOOL)  # CustomSkinsDisabled?
    wrapper.passthrough(BOOL)  # Emote Chat Muted
    wrapper.passthrough(STRING)  # BaseGameVersion
    wrapper.passthrough(INT_LE)  # Limited World Width
    wrapper.passthrough(INT_LE)  # Limited World Depth
    wrapper.passthrough(BOOL)  # Nether type
    wrapper.passthrough(STRING)  # EduSharedUriResource.ButtonName
    wrapper.passthrough(STRING)  # EduSharedUriResource.LinkUri
    wrapper.passthrough(OptionalType(BOOL))  # Override force experimental gameplay
    wrapper.passthrough(BYTE)  # ChatRestriction Level
    wrapper.passthrough(BOOL)  # DisablePlayerInteractions

    level_id = wrapper.read(STRING)  # Level ID
    level_name = wrapper.read(STRING)  # Level Name
    premium_world_template_id = wrapper.read(STRING)  # Template Content Identity
    is_trial = wrapper.read(BOOL)  # Is Trial?
    rewind_history_size = wrapper.read(VAR_INT)  # Movement Settings
    server_auth_block_breaking = wrapper.read(BOOL)  # Movement Settings
    current_tick = wrapper.read(INT64_LE)  # Level Current Time
    enchantment_seed = wrapper.read(VAR_INT)  # Enchantment Seed

    block_properties: list[tuple[str, CompoundTag | None]] = []
    block_property_count = wrapper.read(UVAR_INT)  # Block Properties
    for _ in range(block_property_count):
        block_properties.append((wrapper.read(STRING), wrapper.read(NAMED_COMPOUND_TAG)))

    multiplayer_correlation_id = wrapper.read(STRING)  # Multiplayer Correlation Id
    inventories_server_authoritative = wrapper.read(BOOL)  # Enable Item Stack Net Manager
    server_engine = wrapper.read(STRING)  # Server version
    player_property_data = wrapper.read(NAMED_COMPOUND_TAG)  # Player Property Data
    block_registry_checksum = wrapper.read(INT64_LE)  # Server Block Type Registry Checksum
    world_template_id_msb = wrapper.read(INT64_LE)  # World Template ID
    world_template_id_lsb = wrapper.read(INT64_LE)  # World Template ID
    client_side_generation_enabled = wrapper.read(BOOL)  # Server Enabled ClientSide Generation
    block_network_ids_hashed = wrapper.read(BOOL)  # BlockNetworkIds Are Hashes
    network_permissions = wrapper.read(BOOL)  # NetworkPermissions

    has_server_join_info = wrapper.read(BOOL)  # Does the packet contain server join information.
    if has_server_join_info:
        wrapper.read(BOOL)  # Server Telemetry Data

    server_id = wrapper.read(STRING)  # Server Telemetry Data
    scenario_id = wrapper.read(STRING)  # Server Telemetry Data
    world_id = wrapper.read(STRING)  # Server Telemetry Data
    owner_id = wrapper.read(STRING)  # Server Telemetry Data

    wrapper.write(STRING, server_id)  # Server Telemetry Data
    wrapper.write(STRING, world_id)  # Server Telemetry Data
    wrapper.write(STRING, scenario_id)  # Server Telemetry Data
    wrapper.write(STRING, owner_id)  # Server Telemetry Data

    wrapper.write(STRING, level_id)  # Level ID
    wrapper.write(STRING, level_name)  # Level Name
    wrapper.write(STRING, premium_world_template_id)  # Template Content Identity
    wrapper.write(BOOL, is_trial)  # Is Trial?
    wrapper.write(VAR_INT, rewind_history_size)  # Movement Settings
    wrapper.write(BOOL, server_auth_block_breaking)  # Movement Settings
    wrapper.write(INT64_LE, current_tick)  # Level Current Time
    wrapper.write(VAR_INT, enchantment_seed)  # Enchantment Seed
    wrapper.write(UVAR_INT, len(block_properties))  # Block Properties
    for block_name, block_property_data in block_properties:
        wrapper.write(STRING, block_name)
        wrapper.write(NAMED_COMPOUND_TAG, block_property_data)
    wrapper.write(STRING, multiplayer_correlation_id)  # Multiplayer Correlation Id
    wrapper.write(BOOL, inventories_server_authoritative)  # Enable Item Stack Net Manager
    wrapper.write(STRING, server_engine)  # Server version
    wrapper.write(NAMED_COMPOUND_TAG, player_property_data)  # Player Property Data
    wrapper.write(INT64_LE, block_registry_checksum)  # Server Block Type Registry Checksum
    wrapper.write(INT64_LE, world_template_id_msb)  # World Template ID
    wrapper.write(INT64_LE, world_template_id_lsb)  # World Template ID
    wrapper.write(BOOL, client_side_generation_enabled)  # Server Enabled ClientSide Generation
    wrapper.write(BOOL, block_network_ids_hashed)  # BlockNetworkIds Are Hashes
    wrapper.write(BOOL, network_permissions)  # NetworkPermissions
