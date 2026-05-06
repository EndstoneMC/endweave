"""Handler for StartGamePacket -- v924 server to v898 client."""

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
    CompoundTag,
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
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC2)  # Rotation

    wrapper.passthrough(LEVEL_SETTINGS_V924)

    # -- Read post-settings fields (v924 order: Level ID first, telemetry after checksum) --
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
    wrapper.read(INT64_LE)  # Server Block Type Registry Checksum
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

    # -- Write in v898 order: telemetry first, then Level ID, then checksum fields --
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
    wrapper.write(INT64_LE, 0)  # zero checksum to skip validation
    wrapper.write(INT64_LE, world_template_id_msb)  # World Template ID
    wrapper.write(INT64_LE, world_template_id_lsb)  # World Template ID
    wrapper.write(BOOL, client_side_generation_enabled)  # Server Enabled ClientSide Generation
    wrapper.write(BOOL, block_network_ids_hashed)  # BlockNetworkIds Are Hashes
    wrapper.write(BOOL, network_permissions)  # NetworkPermissions
