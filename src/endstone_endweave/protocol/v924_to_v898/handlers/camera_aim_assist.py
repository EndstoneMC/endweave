"""Handler for CameraAimAssistPresetsPacket -- v924 server to v898 client."""

from ....codec import (
    INT_LE,
    STRING,
    UVAR_INT,
    OptionalType,
    PacketWrapper,
)


def rewrite_camera_aim_assist_presets(wrapper: PacketWrapper) -> None:
    """Drop the v924 entity family arrays.

    Args:
        wrapper: Packet wrapper for CameraAimAssistPresets.
    """
    category_count = wrapper.passthrough(UVAR_INT)  # Camera Aim-Assist Categories
    for _ in range(category_count):
        wrapper.passthrough(STRING)  # Identifier

        count = wrapper.passthrough(UVAR_INT)  # Entity Priorities
        for _ in range(count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(INT_LE)

        count = wrapper.passthrough(UVAR_INT)  # Block Priorities
        for _ in range(count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(INT_LE)

        count = wrapper.passthrough(UVAR_INT)  # Entity Tag Priorities
        for _ in range(count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(INT_LE)

        count = wrapper.read(UVAR_INT)  # Entity Family Priorities (strip for v898)
        for _ in range(count):
            wrapper.read(STRING)
            wrapper.read(INT_LE)

        wrapper.passthrough(OptionalType(INT_LE))  # Default Entity Priority
        wrapper.passthrough(OptionalType(INT_LE))  # Default Block Priority

    preset_count = wrapper.passthrough(UVAR_INT)  # Camera Aim-Assist Presets
    for _ in range(preset_count):
        wrapper.passthrough(STRING)  # Identifier

        block_exclusion_count = wrapper.passthrough(UVAR_INT)  # Block Exclusions
        for _ in range(block_exclusion_count):
            wrapper.passthrough(STRING)

        entity_exclusion_count = wrapper.passthrough(UVAR_INT)  # Entity Exclusions
        for _ in range(entity_exclusion_count):
            wrapper.passthrough(STRING)

        block_tag_exclusion_count = wrapper.passthrough(UVAR_INT)  # Block Tag Exclusions
        for _ in range(block_tag_exclusion_count):
            wrapper.passthrough(STRING)

        entity_type_family_count = wrapper.read(UVAR_INT)  # Entity Type Family Exclusions (strip for v898)
        for _ in range(entity_type_family_count):
            wrapper.read(STRING)

        liquid_count = wrapper.passthrough(UVAR_INT)  # Liquid Targets
        for _ in range(liquid_count):
            wrapper.passthrough(STRING)

        item_count = wrapper.passthrough(UVAR_INT)  # Item Settings
        for _ in range(item_count):
            wrapper.passthrough(STRING)
            wrapper.passthrough(STRING)

        wrapper.passthrough(OptionalType(STRING))  # Default Category for Hands
        wrapper.passthrough(OptionalType(STRING))  # Default Category for Items
