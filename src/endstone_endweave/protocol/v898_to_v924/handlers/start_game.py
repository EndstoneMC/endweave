"""Handler for StartGamePacket -- v898 server to v924 client."""

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
    GameRuleType,
    OptionalType,
    PacketWrapper,
)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Append the v924 StartGame fields missing from v898.

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

    server_id = wrapper.read(STRING)  # Server Telemetry Data
    world_id = wrapper.read(STRING)  # Server Telemetry Data
    scenario_id = wrapper.read(STRING)  # Server Telemetry Data
    owner_id = wrapper.read(STRING)  # Server Telemetry Data

    wrapper.passthrough(STRING)  # Level ID
    wrapper.passthrough(STRING)  # Level Name
    wrapper.passthrough(STRING)  # Template Content Identity
    wrapper.passthrough(BOOL)  # Is Trial?
    wrapper.passthrough(VAR_INT)  # Movement Settings
    wrapper.passthrough(BOOL)  # Movement Settings
    wrapper.passthrough(INT64_LE)  # Level Current Time
    wrapper.passthrough(VAR_INT)  # Enchantment Seed
    block_prop_count = wrapper.passthrough(UVAR_INT)  # Block Properties
    for _ in range(block_prop_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(NAMED_COMPOUND_TAG)
    wrapper.passthrough(STRING)  # Multiplayer Correlation Id
    wrapper.passthrough(BOOL)  # Enable Item Stack Net Manager
    wrapper.passthrough(STRING)  # Server version
    wrapper.passthrough(NAMED_COMPOUND_TAG)  # Player Property Data
    wrapper.passthrough(INT64_LE)  # Server Block Type Registry Checksum
    wrapper.passthrough(INT64_LE)  # World Template ID
    wrapper.passthrough(INT64_LE)  # World Template ID
    wrapper.passthrough(BOOL)  # Server Enabled ClientSide Generation
    wrapper.passthrough(BOOL)  # BlockNetworkIds Are Hashes
    wrapper.passthrough(BOOL)  # NetworkPermissions

    wrapper.write(BOOL, False)  # Does the packet contain server join information.
    wrapper.write(STRING, server_id)  # Server Telemetry Data
    wrapper.write(STRING, scenario_id)  # Server Telemetry Data
    wrapper.write(STRING, world_id)  # Server Telemetry Data
    wrapper.write(STRING, owner_id)  # Server Telemetry Data
