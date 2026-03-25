"""Gameplay packet handlers for v898 server to v860 client translation."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
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
from endstone_endweave.protocol.v860_to_v898.handlers.gameplay import (
    rewrite_animate_clientbound,
    rewrite_animate_serverbound,
)

_MOUSEOVER = 4


def _passthrough_priority(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(STRING)
    wrapper.passthrough(INT_LE)


def _passthrough_item_setting(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)


def _passthrough_experiments(wrapper: PacketWrapper) -> None:
    count = wrapper.passthrough(INT_LE)
    for _ in range(count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(BOOL)


def _passthrough_block_properties(wrapper: PacketWrapper) -> None:
    count = wrapper.passthrough(UVAR_INT)
    for _ in range(count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(NAMED_COMPOUND_TAG)


def rewrite_resource_pack_stack(wrapper: PacketWrapper) -> None:
    """Insert the removed behavior pack array for v860 clients.

    Args:
        wrapper: Packet wrapper for ResourcePackStack.
    """
    wrapper.passthrough(BOOL)
    wrapper.write(UVAR_INT, 0)

    resource_pack_count = wrapper.passthrough(UVAR_INT)
    for _ in range(resource_pack_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)

    wrapper.passthrough(STRING)
    _passthrough_experiments(wrapper)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Insert TickDeathSystemsEnabled back into StartGame.

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
        wrapper.read(BOOL)  # Editable (strip for v860)
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
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)

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
    wrapper.write(BOOL, False)
    wrapper.passthrough(BOOL)


def rewrite_camera_aim_assist_presets(wrapper: PacketWrapper) -> None:
    """Drop the v898 category and exclusion arrays.

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

        block_tag_priority_count = wrapper.read(UVAR_INT)
        for _ in range(block_tag_priority_count):
            _passthrough_priority(wrapper)

        wrapper.passthrough(OptionalType(INT_LE))
        wrapper.passthrough(OptionalType(INT_LE))

    preset_count = wrapper.passthrough(UVAR_INT)
    for _ in range(preset_count):
        wrapper.passthrough(STRING)

        exclusions: list[str] = []

        block_exclusion_count = wrapper.read(UVAR_INT)
        for _ in range(block_exclusion_count):
            exclusions.append(wrapper.read(STRING))

        entity_exclusion_count = wrapper.read(UVAR_INT)
        for _ in range(entity_exclusion_count):
            exclusions.append(wrapper.read(STRING))

        block_tag_exclusion_count = wrapper.read(UVAR_INT)
        for _ in range(block_tag_exclusion_count):
            exclusions.append(wrapper.read(STRING))

        wrapper.write(UVAR_INT, len(exclusions))
        for exclusion in exclusions:
            wrapper.write(STRING, exclusion)

        liquid_count = wrapper.passthrough(UVAR_INT)
        for _ in range(liquid_count):
            wrapper.passthrough(STRING)

        item_setting_count = wrapper.passthrough(UVAR_INT)
        for _ in range(item_setting_count):
            _passthrough_item_setting(wrapper)

        wrapper.passthrough(OptionalType(STRING))
        wrapper.passthrough(OptionalType(STRING))

    wrapper.passthrough(BYTE)


def rewrite_interact(wrapper: PacketWrapper) -> None:
    """Convert the v860 mousePosition into the v898 optional form.

    Args:
        wrapper: Packet wrapper for Interact.
    """
    action = wrapper.passthrough(BYTE)
    wrapper.passthrough(UVAR_INT64)

    if action == _MOUSEOVER:
        wrapper.write(BOOL, True)
        wrapper.passthrough(VEC3)
    else:
        wrapper.write(BOOL, False)


def rewrite_event(wrapper: PacketWrapper) -> None:
    """Drop the extra v898 event header fields.

    Args:
        wrapper: Packet wrapper for Event.
    """
    wrapper.passthrough(VAR_INT64)
    event_type = wrapper.passthrough(VAR_INT)
    wrapper.read(BOOL)
    duplicate_type = wrapper.read(UVAR_INT)
    if duplicate_type != event_type:
        raise ValueError(f"Mismatched event type header: {event_type} != {duplicate_type}")
    wrapper.passthrough_all()


def rewrite_mob_effect(wrapper: PacketWrapper) -> None:
    """Drop the ambient flag from MobEffect.

    Args:
        wrapper: Packet wrapper for MobEffect.
    """
    wrapper.passthrough(UVAR_INT64)
    wrapper.passthrough(BYTE)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(UVAR_INT64)
    wrapper.read(BOOL)


__all__ = [
    "rewrite_animate_clientbound",
    "rewrite_animate_serverbound",
    "rewrite_camera_aim_assist_presets",
    "rewrite_event",
    "rewrite_interact",
    "rewrite_mob_effect",
    "rewrite_resource_pack_stack",
    "rewrite_start_game",
]
