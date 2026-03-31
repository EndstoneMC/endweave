"""Handler for StartGamePacket -- v898 server to v924 client."""

from endstone_endweave.codec import (
    BOOL,
    INT64_LE,
    LEVEL_SETTINGS_V924,
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
    """Append the v924 StartGame fields missing from v898.

    Args:
        wrapper: Packet wrapper for StartGame.
    """
    wrapper.passthrough(VAR_INT64)  # Entity ID
    wrapper.passthrough(UVAR_INT64)  # Runtime ID
    wrapper.passthrough(VAR_INT)  # Game Type
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC2)  # Rotation

    wrapper.passthrough(LEVEL_SETTINGS_V924)

    # -- v898 server telemetry (before Level ID, unlike v924 which has them after) --
    server_id = wrapper.read(STRING)  # Server Telemetry: Server ID
    world_id = wrapper.read(STRING)  # Server Telemetry: World ID
    scenario_id = wrapper.read(STRING)  # Server Telemetry: Scenario ID
    owner_id = wrapper.read(STRING)  # Server Telemetry: Owner ID

    # -- Post-LevelSettings fields (passthrough to v924 client) --
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

    # -- Write v924 server join info + telemetry (after checksum in v924) --
    wrapper.write(BOOL, False)  # Does the packet contain server join information.
    wrapper.write(STRING, server_id)  # Server Telemetry Data
    wrapper.write(STRING, scenario_id)  # Server Telemetry Data
    wrapper.write(STRING, world_id)  # Server Telemetry Data
    wrapper.write(STRING, owner_id)  # Server Telemetry Data
