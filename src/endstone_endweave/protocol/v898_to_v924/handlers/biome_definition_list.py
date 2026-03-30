"""BiomeDefinitionList handler for v898 to v924."""

from endstone_endweave.codec import (
    BIOME_DEFINITION_V898,
    BIOME_DEFINITION_V924,
    USHORT_LE,
    UVAR_INT,
    PacketWrapper,
)


def rewrite_biome_definition_list(wrapper: PacketWrapper) -> None:
    """Appends the v924 villageType field to biome chunk-gen data.

    Args:
        wrapper: Packet wrapper for BiomeDefinitionList.
    """
    biome_count = wrapper.passthrough(UVAR_INT) # Map of Biome names to data
    
    for _ in range(biome_count):
        wrapper.passthrough(USHORT_LE) # String list
        wrapper.map(BIOME_DEFINITION_V898, BIOME_DEFINITION_V924)