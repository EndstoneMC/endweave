"""Gameplay packet handlers for v898 server to v924 client translation."""

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


def _passthrough_item_setting(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Append the v924 StartGame fields missing from v898.

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

    server_id = wrapper.read(STRING)
    world_id = wrapper.read(STRING)
    scenario_id = wrapper.read(STRING)
    owner_id = wrapper.read(STRING)

    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(VAR_INT)
    _passthrough_block_properties(wrapper)
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(NAMED_COMPOUND_TAG)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)

    wrapper.write(BOOL, False)
    wrapper.write(STRING, server_id)
    wrapper.write(STRING, scenario_id)
    wrapper.write(STRING, world_id)
    wrapper.write(STRING, owner_id)


def rewrite_camera_aim_assist_presets(wrapper: PacketWrapper) -> None:
    """Insert the v924 entity family arrays.

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

        wrapper.write(UVAR_INT, 0)

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

        wrapper.write(UVAR_INT, 0)

        liquid_count = wrapper.passthrough(UVAR_INT)
        for _ in range(liquid_count):
            wrapper.passthrough(STRING)

        item_setting_count = wrapper.passthrough(UVAR_INT)
        for _ in range(item_setting_count):
            _passthrough_item_setting(wrapper)

        wrapper.passthrough(OptionalType(STRING))
        wrapper.passthrough(OptionalType(STRING))

    wrapper.passthrough(BYTE)
