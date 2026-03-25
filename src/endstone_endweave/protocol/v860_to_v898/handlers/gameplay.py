"""Gameplay packet handlers for v860 server to v898 client translation."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    EXPERIMENTS_V860,
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

_ENTITY_EVENT_INSERT_AT = 80
_MOUSEOVER = 4


def rewrite_resource_pack_stack(wrapper: PacketWrapper) -> None:
    """Drop the removed behavior pack array from ResourcePackStack.

    Args:
        wrapper: Packet wrapper for ResourcePackStack.
    """
    wrapper.passthrough(BOOL)

    behavior_pack_count = wrapper.read(UVAR_INT)
    for _ in range(behavior_pack_count):
        wrapper.read(STRING)
        wrapper.read(STRING)
        wrapper.read(STRING)

    resource_pack_count = wrapper.passthrough(UVAR_INT)
    for _ in range(resource_pack_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)

    wrapper.passthrough(STRING)
    wrapper.passthrough(EXPERIMENTS_V860)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Drop TickDeathSystemsEnabled from StartGame.

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
        wrapper.passthrough(BOOL)  # Editable
        game_rule_type = wrapper.passthrough(UVAR_INT)
        if game_rule_type == 1:
            wrapper.passthrough(BOOL)
        elif game_rule_type == 2:
            wrapper.passthrough(UVAR_INT)
        elif game_rule_type == 3:
            wrapper.passthrough(FLOAT_LE)
        else:
            raise ValueError(f"Unknown game rule type: {game_rule_type}")

    wrapper.passthrough(EXPERIMENTS_V860)
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
    block_prop_count = wrapper.passthrough(UVAR_INT)
    for _ in range(block_prop_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(NAMED_COMPOUND_TAG)
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(NAMED_COMPOUND_TAG)
    wrapper.read(INT64_LE)
    wrapper.write(INT64_LE, 0)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.read(BOOL)
    wrapper.passthrough(BOOL)


def rewrite_camera_aim_assist_presets(wrapper: PacketWrapper) -> None:
    """Insert the new v898 category and exclusion arrays.

    Args:
        wrapper: Packet wrapper for CameraAimAssistPresets.
    """
    categories_count = wrapper.passthrough(UVAR_INT)
    for _ in range(categories_count):
        wrapper.passthrough(STRING)

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

            wrapper.write(UVAR_INT, 0)

            wrapper.passthrough(OptionalType(INT_LE))
            wrapper.passthrough(OptionalType(INT_LE))

    preset_count = wrapper.passthrough(UVAR_INT)
    for _ in range(preset_count):
        wrapper.passthrough(STRING)

        exclusion_count = wrapper.passthrough(UVAR_INT)
        for _ in range(exclusion_count):
            wrapper.passthrough(STRING)

        wrapper.write(UVAR_INT, 0)
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

    wrapper.passthrough(BYTE)


def rewrite_animate_clientbound(wrapper: PacketWrapper) -> None:
    """Append a missing swingSource option for v898 clients.

    Args:
        wrapper: Packet wrapper for Animate.
    """
    action = wrapper.read(VAR_INT)
    wrapper.write(BYTE, action)
    wrapper.passthrough(UVAR_INT64)
    wrapper.passthrough(FLOAT_LE)
    if action in (128, 129):
        wrapper.read(FLOAT_LE)
    wrapper.write(BOOL, False)


def rewrite_animate_serverbound(wrapper: PacketWrapper) -> None:
    """Drop the v898 swingSource option for v860 servers.

    Args:
        wrapper: Packet wrapper for Animate.
    """
    action = wrapper.read(BYTE)
    wrapper.write(VAR_INT, action)
    wrapper.passthrough(UVAR_INT64)
    wrapper.passthrough(FLOAT_LE)
    if action in (128, 129):
        wrapper.write(FLOAT_LE, 0.0)
    if wrapper.read(BOOL):
        wrapper.read(STRING)


def rewrite_interact(wrapper: PacketWrapper) -> None:
    """Convert v898 optional mousePosition back to the v860 form.

    Args:
        wrapper: Packet wrapper for Interact.
    """
    action = wrapper.passthrough(BYTE)
    wrapper.passthrough(UVAR_INT64)

    has_mouse_position = wrapper.read(BOOL)
    if action == _MOUSEOVER:
        if has_mouse_position:
            wrapper.passthrough(VEC3)
        else:
            wrapper.write(VEC3, (0.0, 0.0, 0.0))
    elif has_mouse_position:
        wrapper.read(VEC3)


def rewrite_event(wrapper: PacketWrapper) -> None:
    """Insert the v898 event header fields.

    Args:
        wrapper: Packet wrapper for Event.
    """
    wrapper.passthrough(VAR_INT64)
    event_type = wrapper.read(VAR_INT)
    wrapper.write(VAR_INT, event_type)
    wrapper.write(BOOL, False)
    wrapper.write(UVAR_INT, event_type)
    wrapper.passthrough_all()


def rewrite_mob_effect(wrapper: PacketWrapper) -> None:
    """Append a missing ambient flag to MobEffect.

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
    wrapper.write(BOOL, False)


def rewrite_actor_event(wrapper: PacketWrapper) -> None:
    """Remap EntityEvent ids for v898 clients.

    Args:
        wrapper: Packet wrapper for EntityEvent.
    """
    wrapper.passthrough(UVAR_INT64)
    event_id = wrapper.read(BYTE)
    remapped = event_id + 1 if event_id >= _ENTITY_EVENT_INSERT_AT else event_id
    wrapper.write(BYTE, remapped)
    wrapper.passthrough(VAR_INT)
