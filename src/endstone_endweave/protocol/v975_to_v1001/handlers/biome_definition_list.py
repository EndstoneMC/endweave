"""BiomeDefinitionList (122) clientbound handler for the v975 -> v1001 delta.

Protocol 1001 (MC 1.26.30) restructured the nested BiomeChunkGeneration body. The
outer packet (Varuint32 biome count + trailing StringList) and the per-biome
header (NameIndex i16, BiomeID i16, 5 floats, MapWaterColour i32, Rain bool,
optional Tags) are UNCHANGED v975<->v1001. Only the Optional[BiomeChunkGeneration]
body changed:

  v975 wrote the surface span INLINE between SurfaceMaterialAdjustments and
  OverworldRules: SurfaceMaterials?, 4 surface bools, MesaSurface?, CappedSurface?.

  v1001 REMOVES that span from the middle and appends two new optionals at the
  tail (after VillageType): SurfaceBuilder (Optional[BiomeSurfaceBuilder]) and
  SubsurfaceBuilder (Optional[BiomeSurfaceBuilder]), where BiomeSurfaceBuilder =
  { SurfaceMaterials?, 4 bools, MesaSurface?, CappedSurface?, NoiseGradientSurface? }.

v944 biome.go == v975 biome.go byte-for-byte (gophertunnel v1.55.2 vs v1.56.2),
and endweave models that body as the *v924* chunk-gen (inline surface span +
trailing village_type), so ``BIOME_DEFINITION_V924`` decodes the real BDS bytes.
This handler reads each biome with that model and re-emits the chunk-gen body in
v1001 wire order, relocating the surface span into a present SurfaceBuilder
(NoiseGradientSurface absent) and writing an absent SubsurfaceBuilder (a v975
server never produces noise-gradient or subsurface data). SurfaceBuilder is
written PRESENT for every biome because the v975 span was always serialised --
this is lossless and matches Mojang's migration of the always-present span.

Field order and array-vs-single for every chunk-gen field were validated against
gophertunnel v1.56.2 minecraft/protocol/biome.go (ReplacementsData and the
surface-adjustment/consolidated/legacy fields are bool-prefixed SLICES; the rest
are single optionals). As a safety net this handler reads the trailing StringList
explicitly and asserts the input is fully consumed: if the decode model is ever
off by a byte the packet raises (caught by the pipeline, which dumps the payload)
rather than silently emitting a malformed list.

Mirrors the shape of v898_to_v924/handlers/biome_definition_list.py.
"""

from endstone_endweave.codec import (
    BIOME_DEFINITION_V924,
    BOOL,
    FLOAT_LE,
    INT_LE,
    SHORT_LE,
    STRING,
    USHORT_LE,
    UVAR_INT,
    PacketWrapper,
)
from endstone_endweave.codec.types.biome import (
    BiomeDefinitionChunkGenData,
    _BIOME_ELEMENT,
    _BIOME_REPLACEMENT_DATA,
    _CAPPED_SURFACE,
    _CLIMATE,
    _CONSOLIDATED_FEATURE,
    _LEGACY_WORLD_GEN_RULES,
    _MESA_SURFACE,
    _MOUNTAIN_PARAMS,
    _MULTINOISE_GEN_RULES,
    _OVERWORLD_GEN_RULES,
    _SURFACE_MATERIAL,
)


def _write_chunk_gen_v1001(wrapper: PacketWrapper, data: BiomeDefinitionChunkGenData) -> None:
    """Serialize a v975-read chunk-gen body in v1001 wire order.

    v1001 order: Climate?, ConsolidatedFeatures?[], MountainParameters?,
    SurfaceMaterialAdjustments?[], OverworldRules?, MultiNoiseRules?, LegacyRules?[],
    ReplacementsData?[], VillageType?, SurfaceBuilder?, SubsurfaceBuilder?. The
    inline v975 surface span migrates into SurfaceBuilder.
    """
    w = wrapper.writer

    # Climate? (single)
    if data.climate is not None:
        BOOL.write(w, True)
        _CLIMATE.write(w, data.climate)
    else:
        BOOL.write(w, False)

    # ConsolidatedFeatures?[] (bool + uvarint count + N)
    if data.consolidated_features is not None:
        BOOL.write(w, True)
        UVAR_INT.write(w, len(data.consolidated_features))
        for cf in data.consolidated_features:
            _CONSOLIDATED_FEATURE.write(w, cf)
    else:
        BOOL.write(w, False)

    # MountainParameters? (single)
    if data.mountain_params is not None:
        BOOL.write(w, True)
        _MOUNTAIN_PARAMS.write(w, data.mountain_params)
    else:
        BOOL.write(w, False)

    # SurfaceMaterialAdjustments?[] (endweave: biome_elements; bool + count + N)
    if data.biome_elements is not None:
        BOOL.write(w, True)
        UVAR_INT.write(w, len(data.biome_elements))
        for be in data.biome_elements:
            _BIOME_ELEMENT.write(w, be)
    else:
        BOOL.write(w, False)

    # --- v975 inline surface span REMOVED here; migrated to SurfaceBuilder below ---

    # OverworldRules? (single)
    if data.overworld_gen_rules is not None:
        BOOL.write(w, True)
        _OVERWORLD_GEN_RULES.write(w, data.overworld_gen_rules)
    else:
        BOOL.write(w, False)

    # MultiNoiseRules? (single)
    if data.multinoise_gen_rules is not None:
        BOOL.write(w, True)
        _MULTINOISE_GEN_RULES.write(w, data.multinoise_gen_rules)
    else:
        BOOL.write(w, False)

    # LegacyRules?[] (endweave wraps the inner slice; bool + (count + N))
    if data.legacy_world_gen_rules is not None:
        BOOL.write(w, True)
        _LEGACY_WORLD_GEN_RULES.write(w, data.legacy_world_gen_rules)
    else:
        BOOL.write(w, False)

    # ReplacementsData?[] (bool + uvarint count + N)
    if data.biome_replacement_data is not None:
        BOOL.write(w, True)
        UVAR_INT.write(w, len(data.biome_replacement_data))
        for brd in data.biome_replacement_data:
            _BIOME_REPLACEMENT_DATA.write(w, brd)
    else:
        BOOL.write(w, False)

    # VillageType? (single optional uint8) -- position unchanged from v975
    if data.village_type is not None:
        BOOL.write(w, True)
        w.write_bytes(bytes((data.village_type & 0xFF,)))
    else:
        BOOL.write(w, False)

    # SurfaceBuilder: Optional[BiomeSurfaceBuilder] -- NEW at v1001. Always present:
    # the v975 surface span is unconditionally serialised, so the faithful v1001 form
    # wraps it in a present SurfaceBuilder.
    BOOL.write(w, True)  # SurfaceBuilder present
    #   SurfaceMaterials? (single)
    if data.surface_material is not None:
        BOOL.write(w, True)
        _SURFACE_MATERIAL.write(w, data.surface_material)
    else:
        BOOL.write(w, False)
    #   4 bools: HasDefaultOverworld / HasSwamp / HasFrozenOcean / HasEnd
    BOOL.write(w, data.flag1)
    BOOL.write(w, data.flag2)
    BOOL.write(w, data.flag3)
    BOOL.write(w, data.flag4)
    #   MesaSurface? (single)
    if data.mesa_surface is not None:
        BOOL.write(w, True)
        _MESA_SURFACE.write(w, data.mesa_surface)
    else:
        BOOL.write(w, False)
    #   CappedSurface? (single)
    if data.capped_surface is not None:
        BOOL.write(w, True)
        _CAPPED_SURFACE.write(w, data.capped_surface)
    else:
        BOOL.write(w, False)
    #   NoiseGradientSurface? -- NEW; a v975 server never produces it -> absent
    BOOL.write(w, False)

    # SubsurfaceBuilder: Optional[BiomeSurfaceBuilder] -- NEW; v975 has none -> absent
    BOOL.write(w, False)


def rewrite_biome_definition_list(wrapper: PacketWrapper) -> None:
    """Restructure each biome's chunk-gen body v975 inline -> v1001 SurfaceBuilder.

    Outer packet (biome count + trailing StringList) and each biome header pass
    through unchanged; only the Optional[BiomeChunkGeneration] body is rewritten
    when present.

    Raises:
        ValueError: if the decoded biome list does not consume exactly up to the
            StringList boundary (model/byte mismatch) -- surfaces a loud failure +
            payload dump instead of emitting a malformed list.
    """
    w = wrapper.writer
    biome_count = wrapper.passthrough(UVAR_INT)

    for _ in range(biome_count):
        # NameIndex (index into the trailing StringList) -- copy through.
        wrapper.passthrough(USHORT_LE)
        # Read the whole biome body in the v975 (== v924) layout.
        biome = wrapper.read(BIOME_DEFINITION_V924)

        # Re-emit the header (unchanged v975/v1001 order).
        SHORT_LE.write(w, biome.short1)  # BiomeID
        FLOAT_LE.write(w, biome.float1)
        FLOAT_LE.write(w, biome.float2)
        FLOAT_LE.write(w, biome.float3)
        FLOAT_LE.write(w, biome.float4)
        FLOAT_LE.write(w, biome.float5)
        INT_LE.write(w, biome.int1)  # MapWaterColour
        BOOL.write(w, biome.flag)  # Rain
        # Tags? (bool + count + ushort[])
        if biome.tags is not None:
            BOOL.write(w, True)
            UVAR_INT.write(w, len(biome.tags))
            for tag in biome.tags:
                USHORT_LE.write(w, tag)
        else:
            BOOL.write(w, False)
        # ChunkGeneration? -- rewritten into v1001 shape.
        if biome.chunk_gen_data is not None:
            BOOL.write(w, True)
            _write_chunk_gen_v1001(wrapper, biome.chunk_gen_data)
        else:
            BOOL.write(w, False)

    # Trailing StringList (shared dictionary), unchanged v975<->v1001. Read it
    # explicitly and copy through so we can assert the whole packet aligned.
    string_count = wrapper.passthrough(UVAR_INT)
    for _ in range(string_count):
        wrapper.passthrough(STRING)

    if wrapper.has_remaining:
        raise ValueError("BiomeDefinitionList v975->v1001: input not fully consumed (decode model misaligned)")
