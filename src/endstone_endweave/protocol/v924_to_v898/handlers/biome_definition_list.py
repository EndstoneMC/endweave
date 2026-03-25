"""BiomeDefinitionList handler for v924 to v898.

Strips the v924 villageType field from biome chunk-gen data by mapping
each biome definition from v924 format (with villageType) to v898 format
(without villageType).
"""

from endstone_endweave.codec import (
    BIOME_DEFINITION_V898,
    BIOME_DEFINITION_V924,
    STRING,
    USHORT_LE,
    UVAR_INT,
    PacketWrapper,
)


def rewrite_biome_definition_list(wrapper: PacketWrapper) -> None:
    """Strip the v924 villageType field from biome chunk-gen data.

    Args:
        wrapper: Packet wrapper for BiomeDefinitionList.
    """
    biome_count = wrapper.passthrough(UVAR_INT)
    for _ in range(biome_count):
        wrapper.passthrough(USHORT_LE)  # String Index to Biome name
        wrapper.map(BIOME_DEFINITION_V924, BIOME_DEFINITION_V898)

    string_count = wrapper.passthrough(UVAR_INT)
    for _ in range(string_count):
        wrapper.passthrough(STRING)
