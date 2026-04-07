"""StartGame packet handler for v898 server to v860 client translation."""

from ....codec import (
    BOOL,
    INT64_LE,
    LEVEL_SETTINGS_V860,
    NAMED_COMPOUND_TAG,
    STRING,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    VEC2,
    VEC3,
    PacketWrapper,
)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Insert TickDeathSystemsEnabled back into StartGame.

    Args:
        wrapper: Packet wrapper for StartGame.
    """
    wrapper.passthrough(VAR_INT64)  # Entity ID
    wrapper.passthrough(UVAR_INT64)  # Runtime ID
    wrapper.passthrough(VAR_INT)  # Game Type
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC2)  # Rotation

    wrapper.passthrough(LEVEL_SETTINGS_V860)

    # -- v898 server telemetry (before Level ID, same position as v860) --
    wrapper.passthrough(STRING)  # Server Telemetry: Server ID
    wrapper.passthrough(STRING)  # Server Telemetry: World ID
    wrapper.passthrough(STRING)  # Server Telemetry: Scenario ID
    wrapper.passthrough(STRING)  # Server Telemetry: Owner ID

    # -- Post-LevelSettings fields --
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
    wrapper.read(INT64_LE)  # Server Block Type Registry Checksum
    wrapper.write(INT64_LE, 0)  # zero checksum to skip validation
    wrapper.passthrough(INT64_LE)  # World Template ID (MSB)
    wrapper.passthrough(INT64_LE)  # World Template ID (LSB)
    wrapper.passthrough(BOOL)  # Server Enabled ClientSide Generation
    wrapper.passthrough(BOOL)  # BlockNetworkIds Are Hashes
    wrapper.write(BOOL, False)  # TickDeathSystemsEnabled (inserted for v860)
