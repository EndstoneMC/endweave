"""Handler for StartGamePacket (11) -- v944 server to v924 client.

v944 changed DefaultSpawn Y from varint to uvarint (BlockPos -> UBlockPos).
v944 also restructured the server join info block near the end of the packet.
"""

from endstone_endweave.codec import (
    BLOCK_PROPERTY,
    BOOL,
    BYTE,
    EXPERIMENTS,
    FLOAT_LE,
    GAME_RULES,
    INT64_LE,
    INT_LE,
    NAMED_COMPOUND_TAG,
    SHORT_LE,
    STRING,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    VEC2,
    VEC3,
    ArrayType,
    PacketWrapper,
)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Rewrite StartGamePacket from v944 to v924 format.

    Args:
        wrapper: Packet wrapper for StartGamePacket.
    """
    wrapper.passthrough(VAR_INT64)  # Entity ID
    wrapper.passthrough(UVAR_INT64)  # Runtime ID
    wrapper.passthrough(VAR_INT)  # Game Type
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC2)  # Rotation
    wrapper.passthrough(INT64_LE)  # Settings.Seed
    wrapper.passthrough(SHORT_LE)  # Settings.SpawnSettings.BiomeType
    wrapper.passthrough(STRING)  # Settings.SpawnSettings.UserDefinedBiomeName
    wrapper.passthrough(VAR_INT)  # Settings.SpawnSettings.Dimension
    wrapper.passthrough(VAR_INT)  # Settings.Generator
    wrapper.passthrough(VAR_INT)  # Settings.GameType
    wrapper.passthrough(BOOL)  # Settings.IsHardcore
    wrapper.passthrough(VAR_INT)  # Settings.GameDifficulty

    # Settings.DefaultSpawn -- Y encoding changed
    wrapper.passthrough(VAR_INT)  # X (same)
    y = wrapper.read(VAR_INT)  # Y: varint in v944
    wrapper.write(UVAR_INT, y)  # Y: uvarint in v924
    wrapper.passthrough(VAR_INT)  # Z (same)

    # -- Remaining LevelSettings (identical in v924/v944) --
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
    wrapper.passthrough(GAME_RULES)  # GameRules
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
    wrapper.passthrough(BOOL)  # Override force experimental gameplay
    wrapper.passthrough(BYTE)  # ChatRestriction Level
    wrapper.passthrough(BOOL)  # DisablePlayerInteractions

    # -- Post-LevelSettings fields (identical in v924/v944) --
    wrapper.passthrough(STRING)  # Level ID
    wrapper.passthrough(STRING)  # Level Name
    wrapper.passthrough(STRING)  # Template Content Identity
    wrapper.passthrough(BOOL)  # Is Trial?
    wrapper.passthrough(VAR_INT)  # Movement Settings.RewindHistorySize
    wrapper.passthrough(BOOL)  # Movement Settings.ServerAuthBlockBreaking
    wrapper.passthrough(INT64_LE)  # Level Current Time
    wrapper.passthrough(VAR_INT)  # Enchantment Seed
    wrapper.passthrough(ArrayType(BLOCK_PROPERTY))  # Block Properties
    wrapper.passthrough(STRING)  # Multiplayer Correlation Id
    wrapper.passthrough(BOOL)  # Enable Item Stack Net Manager
    wrapper.passthrough(STRING)  # Server version
    wrapper.passthrough(NAMED_COMPOUND_TAG)  # Player Property Data
    wrapper.passthrough(INT64_LE)  # Server Block Type Registry Checksum
    wrapper.passthrough(INT64_LE)  # World Template ID (MSB)
    wrapper.passthrough(INT64_LE)  # World Template ID (LSB)
    wrapper.passthrough(BOOL)  # Server Enabled ClientSide Generation
    wrapper.passthrough(BOOL)  # BlockNetworkIds Are Hashes
    wrapper.passthrough(BOOL)  # NetworkPermissions

    # -- Server join info divergence --
    # v944: Does the packet contain server join information. (bool) + v944 structure
    # v924: Does the packet contain server join information. (bool) + v924 structure
    # Strip v944 data and write v924 form
    has_server_join_info = wrapper.passthrough(BOOL)  # Does the packet contain server join information.
    if has_server_join_info:
        wrapper.read(BOOL)  # has gathering join information
        wrapper.read(BOOL)  # has client store entry point information
        wrapper.read(BOOL)  # has presence information
        wrapper.write(BOOL, False)  # has gathering (v924 form)

    # Server Telemetry Data (Social::Events::ServerTelemetryData)
    wrapper.passthrough(STRING)  # Server Telemetry Data[0]
    wrapper.passthrough(STRING)  # Server Telemetry Data[1]
    wrapper.passthrough(STRING)  # Server Telemetry Data[2]
    wrapper.passthrough(STRING)  # Server Telemetry Data[3]
