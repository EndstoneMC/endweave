"""Gameplay packet handlers for v924 server to v898 client translation."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    FLOAT_LE,
    INT64_LE,
    INT_LE,
    NAMED_COMPOUND_TAG,
    SHORT_LE,
    STRING,
    UINT_LE,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    VEC3,
    CompoundTag,
    OptionalType,
    PacketWrapper,
)


def _passthrough_experiments(wrapper: PacketWrapper) -> None:
    count = wrapper.passthrough(UINT_LE)
    for _ in range(count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(BOOL)


def _passthrough_block_properties(wrapper: PacketWrapper) -> None:
    count = wrapper.passthrough(UVAR_INT)
    for _ in range(count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(NAMED_COMPOUND_TAG)


def _passthrough_priority(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(STRING)
    wrapper.passthrough(INT_LE)


def _read_priority(wrapper: PacketWrapper) -> None:
    wrapper.read(STRING)
    wrapper.read(INT_LE)


def _passthrough_item_setting(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Strip the v924 StartGame additions that v898 does not understand.

    Args:
        wrapper: Packet wrapper for StartGame.
    """
    wrapper.passthrough(VAR_INT64)
    wrapper.passthrough(UVAR_INT64)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)

    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(STRING)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)

    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)

    game_rule_count = wrapper.passthrough(UVAR_INT)
    for _ in range(game_rule_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(BOOL)
        game_rule_type = wrapper.passthrough(UVAR_INT)
        if game_rule_type == 1:
            wrapper.passthrough(BOOL)
        elif game_rule_type == 2:
            wrapper.passthrough(VAR_INT)
        elif game_rule_type == 3:
            wrapper.passthrough(FLOAT_LE)
        else:
            raise ValueError(f"Unknown game rule type: {game_rule_type}")

    _passthrough_experiments(wrapper)
    wrapper.passthrough(BOOL)

    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(OptionalType(BOOL))
    wrapper.passthrough(BYTE)
    wrapper.passthrough(BOOL)

    level_id = wrapper.read(STRING)
    level_name = wrapper.read(STRING)
    premium_world_template_id = wrapper.read(STRING)
    is_trial = wrapper.read(BOOL)
    rewind_history_size = wrapper.read(VAR_INT)
    server_auth_block_breaking = wrapper.read(BOOL)
    current_tick = wrapper.read(INT64_LE)
    enchantment_seed = wrapper.read(VAR_INT)

    block_properties: list[tuple[str, CompoundTag | None]] = []
    block_property_count = wrapper.read(UVAR_INT)
    for _ in range(block_property_count):
        block_properties.append((wrapper.read(STRING), wrapper.read(NAMED_COMPOUND_TAG)))

    multiplayer_correlation_id = wrapper.read(STRING)
    inventories_server_authoritative = wrapper.read(BOOL)
    server_engine = wrapper.read(STRING)
    player_property_data = wrapper.read(NAMED_COMPOUND_TAG)
    block_registry_checksum = wrapper.read(INT64_LE)
    world_template_id_msb = wrapper.read(INT64_LE)
    world_template_id_lsb = wrapper.read(INT64_LE)
    client_side_generation_enabled = wrapper.read(BOOL)
    block_network_ids_hashed = wrapper.read(BOOL)
    network_permissions = wrapper.read(BOOL)

    has_server_join_info = wrapper.read(BOOL)
    if has_server_join_info:
        wrapper.read(BOOL)

    server_id = wrapper.read(STRING)
    scenario_id = wrapper.read(STRING)
    world_id = wrapper.read(STRING)
    owner_id = wrapper.read(STRING)

    wrapper.write(STRING, server_id)
    wrapper.write(STRING, world_id)
    wrapper.write(STRING, scenario_id)
    wrapper.write(STRING, owner_id)

    wrapper.write(STRING, level_id)
    wrapper.write(STRING, level_name)
    wrapper.write(STRING, premium_world_template_id)
    wrapper.write(BOOL, is_trial)
    wrapper.write(VAR_INT, rewind_history_size)
    wrapper.write(BOOL, server_auth_block_breaking)
    wrapper.write(INT64_LE, current_tick)
    wrapper.write(VAR_INT, enchantment_seed)
    wrapper.write(UVAR_INT, len(block_properties))
    for block_name, block_property_data in block_properties:
        wrapper.write(STRING, block_name)
        wrapper.write(NAMED_COMPOUND_TAG, block_property_data)
    wrapper.write(STRING, multiplayer_correlation_id)
    wrapper.write(BOOL, inventories_server_authoritative)
    wrapper.write(STRING, server_engine)
    wrapper.write(NAMED_COMPOUND_TAG, player_property_data)
    wrapper.write(INT64_LE, block_registry_checksum)
    wrapper.write(INT64_LE, world_template_id_msb)
    wrapper.write(INT64_LE, world_template_id_lsb)
    wrapper.write(BOOL, client_side_generation_enabled)
    wrapper.write(BOOL, block_network_ids_hashed)
    wrapper.write(BOOL, network_permissions)


def rewrite_camera_aim_assist_presets(wrapper: PacketWrapper) -> None:
    """Drop the v924 entity family arrays.

    Args:
        wrapper: Packet wrapper for CameraAimAssistPresets.
    """
    category_count = wrapper.passthrough(UVAR_INT)
    for _ in range(category_count):
        wrapper.passthrough(STRING)

        entity_priority_count = wrapper.passthrough(UVAR_INT)
        for _ in range(entity_priority_count):
            _passthrough_priority(wrapper)

        block_priority_count = wrapper.passthrough(UVAR_INT)
        for _ in range(block_priority_count):
            _passthrough_priority(wrapper)

        block_tag_priority_count = wrapper.passthrough(UVAR_INT)
        for _ in range(block_tag_priority_count):
            _passthrough_priority(wrapper)

        entity_type_family_count = wrapper.read(UVAR_INT)
        for _ in range(entity_type_family_count):
            _read_priority(wrapper)

        wrapper.passthrough(OptionalType(INT_LE))
        wrapper.passthrough(OptionalType(INT_LE))

    preset_count = wrapper.passthrough(UVAR_INT)
    for _ in range(preset_count):
        wrapper.passthrough(STRING)

        block_exclusion_count = wrapper.passthrough(UVAR_INT)
        for _ in range(block_exclusion_count):
            wrapper.passthrough(STRING)

        entity_exclusion_count = wrapper.passthrough(UVAR_INT)
        for _ in range(entity_exclusion_count):
            wrapper.passthrough(STRING)

        block_tag_exclusion_count = wrapper.passthrough(UVAR_INT)
        for _ in range(block_tag_exclusion_count):
            wrapper.passthrough(STRING)

        entity_type_family_count = wrapper.read(UVAR_INT)
        for _ in range(entity_type_family_count):
            wrapper.read(STRING)

        liquid_count = wrapper.passthrough(UVAR_INT)
        for _ in range(liquid_count):
            wrapper.passthrough(STRING)

        item_setting_count = wrapper.passthrough(UVAR_INT)
        for _ in range(item_setting_count):
            _passthrough_item_setting(wrapper)

        wrapper.passthrough(OptionalType(STRING))
        wrapper.passthrough(OptionalType(STRING))

    wrapper.passthrough(BYTE)


def rewrite_graphics_parameter_override(wrapper: PacketWrapper) -> None:
    """Strip the v924 float/vector override payloads.

    Args:
        wrapper: Packet wrapper for GraphicsParameterOverride.
    """
    value_count = wrapper.passthrough(UVAR_INT)
    for _ in range(value_count):
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(VEC3)

    if wrapper.read(BOOL):
        wrapper.read(FLOAT_LE)

    if wrapper.read(BOOL):
        wrapper.read(VEC3)

    wrapper.passthrough(STRING)
    wrapper.passthrough(BYTE)
    wrapper.passthrough(BOOL)
