"""Gameplay packet handlers for v898 server to v924 client translation."""

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
    VEC3,
    OptionalType,
    PacketWrapper,
)


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

    wrapper.passthrough(EXPERIMENTS)
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
    block_prop_count = wrapper.passthrough(UVAR_INT)
    for _ in range(block_prop_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(NAMED_COMPOUND_TAG)
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

        count = wrapper.passthrough(UVAR_INT)
        for _ in range(count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(INT_LE)

        count = wrapper.passthrough(UVAR_INT)
        for _ in range(count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(INT_LE)

        count = wrapper.passthrough(UVAR_INT)
        for _ in range(count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(INT_LE)

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

        item_count = wrapper.passthrough(UVAR_INT)
        for _ in range(item_count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(STRING)

        wrapper.passthrough(OptionalType(STRING))
        wrapper.passthrough(OptionalType(STRING))


def rewrite_graphics_parameter_override(wrapper: PacketWrapper) -> None:
    """Insert the v924 Float Value and Vec3 Value optional fields.

    Args:
        wrapper: Packet wrapper for GraphicsParameterOverride.
    """
    value_count = wrapper.passthrough(UVAR_INT)
    for _ in range(value_count):
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(VEC3)

    wrapper.write(BOOL, False)  # Float Value (not present)
    wrapper.write(BOOL, False)  # Vec3 Value (not present)
