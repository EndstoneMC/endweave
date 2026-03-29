"""CameraAimAssistPresets packet handler for v898 server to v860 client translation."""

from endstone_endweave.codec import (
    INT_LE,
    STRING,
    UVAR_INT,
    OptionalType,
    PacketWrapper,
)


def rewrite_camera_aim_assist_presets(wrapper: PacketWrapper) -> None:
    """Drop the v898 category and exclusion arrays.

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

        block_tag_priority_count = wrapper.read(UVAR_INT)
        for _ in range(block_tag_priority_count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(INT_LE)

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

        item_count = wrapper.passthrough(UVAR_INT)
        for _ in range(item_count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(STRING)

        wrapper.passthrough(OptionalType(STRING))
        wrapper.passthrough(OptionalType(STRING))
