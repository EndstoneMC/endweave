"""Handler for CameraAimAssistPresetsPacket -- v860 server to v898 client."""

from endstone_endweave.codec import (
    INT_LE,
    STRING,
    UVAR_INT,
    OptionalType,
    PacketWrapper,
)


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
