"""Handler for StartGamePacket (11) -- v924 server to v944 client.

v944 changed DefaultSpawn Y from uvarint to varint (UBlockPos -> BlockPos).
v944 also restructured the server join info block near the end of the packet.
We passthrough all identical fields, convert the Y encoding,
strip the v924 server join info, and write false for v944's has_server_join_info.
"""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    COMPOUND_TAG,
    FLOAT_LE,
    INT_LE,
    LONG_LE,
    SHORT_LE,
    STRING,
    UINT_LE,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
)


def _passthrough_game_rules(wrapper: PacketWrapper) -> None:
    """Passthrough GameRules: uvarint count + per-rule entries."""
    count = wrapper.passthrough(UVAR_INT)
    for _ in range(count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(BOOL)  # editable
        type_id = wrapper.passthrough(UVAR_INT)  # rule type
        if type_id == 1:  # bool
            wrapper.passthrough(BOOL)
        elif type_id == 2:  # varint
            wrapper.passthrough(VAR_INT)
        elif type_id == 3:  # float
            wrapper.passthrough(FLOAT_LE)
        else:
            raise ValueError(f"Unknown game rule type: {type_id}")


def _passthrough_experiments(wrapper: PacketWrapper) -> None:
    """Passthrough Experiments: uint32_le count + entries + ever_toggled bool."""
    count = wrapper.passthrough(UINT_LE)
    for _ in range(count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(BOOL)  # enabled
    wrapper.passthrough(BOOL)  # ever_toggled


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Rewrite StartGamePacket from v924 to v944 format."""
    wrapper.passthrough(VAR_INT64)  # mEntityId
    wrapper.passthrough(UVAR_INT64)  # mRuntimeId
    wrapper.passthrough(VAR_INT)  # mEntityGameType
    wrapper.passthrough(FLOAT_LE)  # mPos.X
    wrapper.passthrough(FLOAT_LE)  # mPos.Y
    wrapper.passthrough(FLOAT_LE)  # mPos.Z
    wrapper.passthrough(FLOAT_LE)  # mRot.X
    wrapper.passthrough(FLOAT_LE)  # mRot.Y
    wrapper.passthrough(LONG_LE)  # mLevelSettings.mSeed
    wrapper.passthrough(SHORT_LE)  # mLevelSettings.mSpawnSettings.type
    wrapper.passthrough(STRING)  # mLevelSettings.mSpawnSettings.userDefinedBiomeName
    wrapper.passthrough(VAR_INT)  # mLevelSettings.mSpawnSettings.dimension
    wrapper.passthrough(VAR_INT)  # mLevelSettings.mGenerator
    wrapper.passthrough(VAR_INT)  # mLevelSettings.mGameType
    wrapper.passthrough(BOOL)  # mLevelSettings.mIsHardcore
    wrapper.passthrough(VAR_INT)  # mLevelSettings.mGameDifficulty

    # mLevelSettings.mDefaultSpawn -- Y encoding changed
    wrapper.passthrough(VAR_INT)  # X (same)
    y = wrapper.read(UVAR_INT)  # Y: uvarint in v924
    wrapper.write(VAR_INT, y)  # Y: varint in v944
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
    _passthrough_game_rules(wrapper)  # GameRules
    _passthrough_experiments(wrapper)  # Experiments
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
    wrapper.passthrough(BOOL)  # Is Trial
    wrapper.passthrough(VAR_INT)  # SyncedPlayerMovementSettings.RewindHistorySize
    wrapper.passthrough(BOOL)  # SyncedPlayerMovementSettings.ServerAuthBlockBreaking
    wrapper.passthrough(LONG_LE)  # Level Current Time
    wrapper.passthrough(VAR_INT)  # Enchantment Seed

    # Block Properties: uvarint count + [string + CompoundTag][]
    block_prop_count = wrapper.passthrough(UVAR_INT)
    for _ in range(block_prop_count):
        wrapper.passthrough(STRING)  # block name
        wrapper.passthrough(COMPOUND_TAG)  # block properties NBT

    wrapper.passthrough(STRING)  # Multiplayer Correlation Id
    wrapper.passthrough(BOOL)  # Enable Item Stack Net Manager
    wrapper.passthrough(STRING)  # Server version
    wrapper.passthrough(COMPOUND_TAG)  # Player Property Data
    wrapper.read(LONG_LE)  # Server Block Type Registry Checksum
    wrapper.write(LONG_LE, 0)  # zero checksum to skip validation
    wrapper.passthrough(LONG_LE)  # World Template ID MSB
    wrapper.passthrough(LONG_LE)  # World Template ID LSB
    wrapper.passthrough(BOOL)  # Server Enabled ClientSide Generation
    wrapper.passthrough(BOOL)  # BlockNetworkIds Are Hashes
    wrapper.passthrough(BOOL)  # serverAuthSoundEnabled (NetworkPermissions)

    # -- Server join info divergence --
    # v924: has_server_join_info (bool) + optional gathering data
    # v944: has_server_join_info (bool) + different structure
    # Strip v924 data and write false for v944
    has_server_join_info = wrapper.passthrough(BOOL)
    if has_server_join_info:
        has_gathering = wrapper.read(BOOL)
        if has_gathering:
            for _ in range(7):
                wrapper.read(STRING)
        wrapper.write(BOOL, False)  # has gathering join information
        wrapper.write(BOOL, False)  # has client store entry point information
        wrapper.write(BOOL, False)  # has presence information

    # Trailing strings (identical in both versions)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
