"""endweave CLIENTBOUND StartGame handler for the v975 -> v1001 delta.

Chain for a 1.26.30 (protocol v1001) client is:  v1001 <- v975 <- v944.
This handler receives a *v975-format* StartGame (already produced by the
v944_to_v975 clientbound handler, which zeroes the block-registry checksum) and
rewrites it into the *v1001 wire format* a 1.26.30 client expects.

Ground truth (cross-checked byte-for-byte, CloudburstMC/Protocol branch 3.0
StartGameSerializer_v1001 chain AND Sandertv/gophertunnel start_game.go @1.26.30):
v1001 adds exactly THREE new scalar fields relative to the v944/v975 format:

  1. serverEditorConnectionPolicy        VAR_INT -- appended in writeLevelSettings
       immediately AFTER disablingPlayerInteractions (last LEVEL_SETTINGS_V944 field).
  2. allowAnonymousBlockDropsInEditorWorlds  BOOL -- appended immediately AFTER it
       (last level-settings field; next written is levelId).
  3. isLoggingChat                        BOOL -- inserted in writeBeforeNetworkPermissions
       immediately AFTER serverAuthoritativeSound and BEFORE serverConfigurationJoinInfo.

History trap (verified, do NOT "fix"): v827 inserted tickDeathSystemsEnabled before
serverAuthSound; v898 reverted it. v1001's chain routes through v898, so that field is
ABSENT in both the v975 input and the v1001 output -- this handler correctly omits it.
"""

from endstone_endweave.codec import (
    BOOL,
    INT64_LE,
    LEVEL_SETTINGS_V944,
    NAMED_COMPOUND_TAG,
    STRING,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    VEC2,
    VEC3,
    UUID,
    PacketWrapper,
)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Upgrade a v975-format clientbound StartGame to v1001 wire format."""

    wrapper.passthrough(VAR_INT64)  # Entity ID
    wrapper.passthrough(UVAR_INT64)  # Runtime ID
    wrapper.passthrough(VAR_INT)  # Game Type
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC2)  # Rotation

    # Read LevelSettings (v975 layout)
    settings = wrapper.read(LEVEL_SETTINGS_V944)
    # Write LevelSettings
    wrapper.write(LEVEL_SETTINGS_V944, settings)

    # Inject the 2 new LevelSettings fields introduced in v1001
    wrapper.write(VAR_INT, 0)  # ServerEditorConnectionPolicy
    wrapper.write(BOOL, False)  # AllowAnonymousBlockDropsInEditorWorlds

    wrapper.passthrough(STRING)  # Level ID
    wrapper.passthrough(STRING)  # Level Name
    wrapper.passthrough(STRING)  # Template Content Identity
    wrapper.passthrough(BOOL)  # Is Trial?
    wrapper.passthrough(VAR_INT)  # Movement Settings.RewindHistorySize
    wrapper.passthrough(BOOL)  # Movement Settings.ServerAuthBlockBreaking
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

    # Zero checksum
    wrapper.read(INT64_LE)  # Server Block Type Registry Checksum
    wrapper.write(INT64_LE, 0)  # zero checksum to skip validation

    # Passthrough the fields before IsLoggingChat
    wrapper.passthrough(UUID)  # World Template ID
    wrapper.passthrough(BOOL)  # Client Side Generation
    wrapper.passthrough(BOOL)  # Use Block Network ID Hashes
    wrapper.passthrough(BOOL)  # Server Authoritative Sound

    # Inject the new IsLoggingChat field introduced in v1001
    wrapper.write(BOOL, False)  # IsLoggingChat

    wrapper.passthrough_all()  # remaining fields are wire-identical
