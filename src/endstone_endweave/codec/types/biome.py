"""Biome generation compound types for BiomeDefinitionList packet translation.

All inner structures are shared between v898 and v924. The only difference is
BiomeDefinitionChunkGenData: v924 appends a trailing village_type field (BOOL + BYTE) that
v898 does not have. BiomeDefinitionData delegates to the appropriate BiomeDefinitionChunkGenData
variant.
"""

from dataclasses import dataclass, field

from ..reader import PacketReader
from ..writer import PacketWriter
from .primitives import (
    BOOL,
    BYTE,
    FLOAT_LE,
    INT_LE,
    SHORT_LE,
    USHORT_LE,
    UVAR_INT,
    VAR_INT,
    Type,
)

# ---------------------------------------------------------------------------
# BiomeClimateData
# ---------------------------------------------------------------------------


@dataclass
class BiomeClimateData:
    """Biome climate parameters."""

    temperature: float
    downfall: float
    ash: float
    red_spores: float


class _BiomeClimateDataType(Type["BiomeClimateData"]):
    """4 FLOAT_LE fields."""

    def read(self, reader: PacketReader) -> BiomeClimateData:
        return BiomeClimateData(
            temperature=FLOAT_LE.read(reader),
            downfall=FLOAT_LE.read(reader),
            ash=FLOAT_LE.read(reader),
            red_spores=FLOAT_LE.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeClimateData) -> None:
        FLOAT_LE.write(writer, value.temperature)
        FLOAT_LE.write(writer, value.downfall)
        FLOAT_LE.write(writer, value.ash)
        FLOAT_LE.write(writer, value.red_spores)


_CLIMATE = _BiomeClimateDataType()

# ---------------------------------------------------------------------------
# BiomeWeightedData
# ---------------------------------------------------------------------------


@dataclass
class BiomeWeightedData:
    """A weighted block entry."""

    weight: int
    block: int


class _BiomeWeightedDataType(Type["BiomeWeightedData"]):
    """SHORT_LE + INT_LE pair."""

    def read(self, reader: PacketReader) -> BiomeWeightedData:
        return BiomeWeightedData(
            weight=SHORT_LE.read(reader),
            block=INT_LE.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeWeightedData) -> None:
        SHORT_LE.write(writer, value.weight)
        INT_LE.write(writer, value.block)


_WEIGHT = _BiomeWeightedDataType()

# ---------------------------------------------------------------------------
# BiomeWeightedTemperatureData
# ---------------------------------------------------------------------------


@dataclass
class BiomeWeightedTemperatureData:
    """A weighted temperature entry."""

    temperature: int
    weight: int


class _BiomeWeightedTemperatureDataType(Type["BiomeWeightedTemperatureData"]):
    """VAR_INT + INT_LE pair."""

    def read(self, reader: PacketReader) -> BiomeWeightedTemperatureData:
        return BiomeWeightedTemperatureData(
            temperature=VAR_INT.read(reader),
            weight=INT_LE.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeWeightedTemperatureData) -> None:
        VAR_INT.write(writer, value.temperature)
        INT_LE.write(writer, value.weight)


_WEIGHTED_TEMPERATURE = _BiomeWeightedTemperatureDataType()

# ---------------------------------------------------------------------------
# BiomeCoordinateData
# ---------------------------------------------------------------------------


@dataclass
class BiomeCoordinateData:
    """A biome generation coordinate expression."""

    expr_op1: int
    short1: int
    expr_op2: int
    short2: int
    int1: int
    int2: int
    varint: int


class _BiomeCoordinateDataType(Type["BiomeCoordinateData"]):
    """VAR_INT, SHORT_LE, VAR_INT, SHORT_LE, INT_LE, INT_LE, VAR_INT."""

    def read(self, reader: PacketReader) -> BiomeCoordinateData:
        return BiomeCoordinateData(
            expr_op1=VAR_INT.read(reader),
            short1=SHORT_LE.read(reader),
            expr_op2=VAR_INT.read(reader),
            short2=SHORT_LE.read(reader),
            int1=INT_LE.read(reader),
            int2=INT_LE.read(reader),
            varint=VAR_INT.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeCoordinateData) -> None:
        VAR_INT.write(writer, value.expr_op1)
        SHORT_LE.write(writer, value.short1)
        VAR_INT.write(writer, value.expr_op2)
        SHORT_LE.write(writer, value.short2)
        INT_LE.write(writer, value.int1)
        INT_LE.write(writer, value.int2)
        VAR_INT.write(writer, value.varint)


_COORDINATE = _BiomeCoordinateDataType()

# ---------------------------------------------------------------------------
# BiomeSurfaceMaterialData
# ---------------------------------------------------------------------------


@dataclass
class BiomeSurfaceMaterialData:
    """Surface material block palette."""

    block1: int
    block2: int
    block3: int
    block4: int
    block5: int
    extra: int


class _BiomeSurfaceMaterialDataType(Type["BiomeSurfaceMaterialData"]):
    """5x INT_LE blocks + INT_LE extra."""

    def read(self, reader: PacketReader) -> BiomeSurfaceMaterialData:
        return BiomeSurfaceMaterialData(
            block1=INT_LE.read(reader),
            block2=INT_LE.read(reader),
            block3=INT_LE.read(reader),
            block4=INT_LE.read(reader),
            block5=INT_LE.read(reader),
            extra=INT_LE.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeSurfaceMaterialData) -> None:
        INT_LE.write(writer, value.block1)
        INT_LE.write(writer, value.block2)
        INT_LE.write(writer, value.block3)
        INT_LE.write(writer, value.block4)
        INT_LE.write(writer, value.block5)
        INT_LE.write(writer, value.extra)


_SURFACE_MATERIAL = _BiomeSurfaceMaterialDataType()

# ---------------------------------------------------------------------------
# BiomeElementData
# ---------------------------------------------------------------------------


@dataclass
class BiomeElementData:
    """A biome height/surface element."""

    float1: float
    float2: float
    float3: float
    expr_op1: int
    short1: int
    expr_op2: int
    short2: int
    surface_material: BiomeSurfaceMaterialData


class _BiomeElementDataType(Type["BiomeElementData"]):
    """3 FLOAT_LE + 2x (VAR_INT + SHORT_LE) + BiomeSurfaceMaterialData."""

    def read(self, reader: PacketReader) -> BiomeElementData:
        return BiomeElementData(
            float1=FLOAT_LE.read(reader),
            float2=FLOAT_LE.read(reader),
            float3=FLOAT_LE.read(reader),
            expr_op1=VAR_INT.read(reader),
            short1=SHORT_LE.read(reader),
            expr_op2=VAR_INT.read(reader),
            short2=SHORT_LE.read(reader),
            surface_material=_SURFACE_MATERIAL.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeElementData) -> None:
        FLOAT_LE.write(writer, value.float1)
        FLOAT_LE.write(writer, value.float2)
        FLOAT_LE.write(writer, value.float3)
        VAR_INT.write(writer, value.expr_op1)
        SHORT_LE.write(writer, value.short1)
        VAR_INT.write(writer, value.expr_op2)
        SHORT_LE.write(writer, value.short2)
        _SURFACE_MATERIAL.write(writer, value.surface_material)


_BIOME_ELEMENT = _BiomeElementDataType()

# ---------------------------------------------------------------------------
# BiomeScatterParamData
# ---------------------------------------------------------------------------


@dataclass
class BiomeScatterParamData:
    """Scatter parameters for consolidated features.

    Attributes:
        coordinates: List of coordinate expressions.
        varint1: First scatter parameter.
        varint2: Second scatter parameter.
        short1: Short parameter.
        int1: First int parameter.
        int2: Second int parameter.
        varint3: Third scatter parameter.
        short2: Short parameter.
    """

    coordinates: list[BiomeCoordinateData] = field(default_factory=list)
    varint1: int = 0
    varint2: int = 0
    short1: int = 0
    int1: int = 0
    int2: int = 0
    varint3: int = 0
    short2: int = 0


class _BiomeScatterParamDataType(Type["BiomeScatterParamData"]):
    """Array of BiomeCoordinateData + 3 VAR_INT + 2 SHORT_LE + 2 INT_LE."""

    def read(self, reader: PacketReader) -> BiomeScatterParamData:
        count = UVAR_INT.read(reader)
        coordinates = [_COORDINATE.read(reader) for _ in range(count)]
        return BiomeScatterParamData(
            coordinates=coordinates,
            varint1=VAR_INT.read(reader),
            varint2=VAR_INT.read(reader),
            short1=SHORT_LE.read(reader),
            int1=INT_LE.read(reader),
            int2=INT_LE.read(reader),
            varint3=VAR_INT.read(reader),
            short2=SHORT_LE.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeScatterParamData) -> None:
        UVAR_INT.write(writer, len(value.coordinates))
        for coord in value.coordinates:
            _COORDINATE.write(writer, coord)
        VAR_INT.write(writer, value.varint1)
        VAR_INT.write(writer, value.varint2)
        SHORT_LE.write(writer, value.short1)
        INT_LE.write(writer, value.int1)
        INT_LE.write(writer, value.int2)
        VAR_INT.write(writer, value.varint3)
        SHORT_LE.write(writer, value.short2)


_SCATTER_PARAM = _BiomeScatterParamDataType()

# ---------------------------------------------------------------------------
# BiomeConsolidatedFeatureData
# ---------------------------------------------------------------------------


@dataclass
class BiomeConsolidatedFeatureData:
    """A consolidated biome feature entry."""

    scatter_param: BiomeScatterParamData
    short1: int
    short2: int
    short3: int
    flag: bool


class _BiomeConsolidatedFeatureDataType(Type["BiomeConsolidatedFeatureData"]):
    """BiomeScatterParamData + 3 SHORT_LE + BOOL."""

    def read(self, reader: PacketReader) -> BiomeConsolidatedFeatureData:
        return BiomeConsolidatedFeatureData(
            scatter_param=_SCATTER_PARAM.read(reader),
            short1=SHORT_LE.read(reader),
            short2=SHORT_LE.read(reader),
            short3=SHORT_LE.read(reader),
            flag=BOOL.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeConsolidatedFeatureData) -> None:
        _SCATTER_PARAM.write(writer, value.scatter_param)
        SHORT_LE.write(writer, value.short1)
        SHORT_LE.write(writer, value.short2)
        SHORT_LE.write(writer, value.short3)
        BOOL.write(writer, value.flag)


_CONSOLIDATED_FEATURE = _BiomeConsolidatedFeatureDataType()

# ---------------------------------------------------------------------------
# BiomeMountainParamsData
# ---------------------------------------------------------------------------


@dataclass
class BiomeMountainParamsData:
    """Mountain generation parameters."""

    block: int
    flag1: bool
    flag2: bool
    flag3: bool
    flag4: bool
    flag5: bool


class _BiomeMountainParamsDataType(Type["BiomeMountainParamsData"]):
    """INT_LE block + 5 BOOLs."""

    def read(self, reader: PacketReader) -> BiomeMountainParamsData:
        return BiomeMountainParamsData(
            block=INT_LE.read(reader),
            flag1=BOOL.read(reader),
            flag2=BOOL.read(reader),
            flag3=BOOL.read(reader),
            flag4=BOOL.read(reader),
            flag5=BOOL.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeMountainParamsData) -> None:
        INT_LE.write(writer, value.block)
        BOOL.write(writer, value.flag1)
        BOOL.write(writer, value.flag2)
        BOOL.write(writer, value.flag3)
        BOOL.write(writer, value.flag4)
        BOOL.write(writer, value.flag5)


_MOUNTAIN_PARAMS = _BiomeMountainParamsDataType()

# ---------------------------------------------------------------------------
# BiomeMesaSurfaceData
# ---------------------------------------------------------------------------


@dataclass
class BiomeMesaSurfaceData:
    """Mesa surface parameters."""

    block1: int
    block2: int
    flag1: bool
    flag2: bool


class _BiomeMesaSurfaceDataType(Type["BiomeMesaSurfaceData"]):
    """2 INT_LE blocks + 2 BOOLs."""

    def read(self, reader: PacketReader) -> BiomeMesaSurfaceData:
        return BiomeMesaSurfaceData(
            block1=INT_LE.read(reader),
            block2=INT_LE.read(reader),
            flag1=BOOL.read(reader),
            flag2=BOOL.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeMesaSurfaceData) -> None:
        INT_LE.write(writer, value.block1)
        INT_LE.write(writer, value.block2)
        BOOL.write(writer, value.flag1)
        BOOL.write(writer, value.flag2)


_MESA_SURFACE = _BiomeMesaSurfaceDataType()

# ---------------------------------------------------------------------------
# BiomeCappedSurfaceData
# ---------------------------------------------------------------------------


@dataclass
class BiomeCappedSurfaceData:
    """Capped surface parameters.

    Attributes:
        blocks1: First block array.
        blocks2: Second block array.
        optional_block1: Optional block 1.
        optional_block2: Optional block 2.
        optional_block3: Optional block 3.
    """

    blocks1: list[int] = field(default_factory=list)
    blocks2: list[int] = field(default_factory=list)
    optional_block1: int | None = None
    optional_block2: int | None = None
    optional_block3: int | None = None


class _BiomeCappedSurfaceDataType(Type["BiomeCappedSurfaceData"]):
    """2 block arrays + 3 optional blocks."""

    def read(self, reader: PacketReader) -> BiomeCappedSurfaceData:
        count1 = UVAR_INT.read(reader)
        blocks1 = [INT_LE.read(reader) for _ in range(count1)]
        count2 = UVAR_INT.read(reader)
        blocks2 = [INT_LE.read(reader) for _ in range(count2)]
        optional_block1 = INT_LE.read(reader) if BOOL.read(reader) else None
        optional_block2 = INT_LE.read(reader) if BOOL.read(reader) else None
        optional_block3 = INT_LE.read(reader) if BOOL.read(reader) else None
        return BiomeCappedSurfaceData(
            blocks1=blocks1,
            blocks2=blocks2,
            optional_block1=optional_block1,
            optional_block2=optional_block2,
            optional_block3=optional_block3,
        )

    def write(self, writer: PacketWriter, value: BiomeCappedSurfaceData) -> None:
        UVAR_INT.write(writer, len(value.blocks1))
        for block in value.blocks1:
            INT_LE.write(writer, block)
        UVAR_INT.write(writer, len(value.blocks2))
        for block in value.blocks2:
            INT_LE.write(writer, block)
        if value.optional_block1 is not None:
            BOOL.write(writer, True)
            INT_LE.write(writer, value.optional_block1)
        else:
            BOOL.write(writer, False)
        if value.optional_block2 is not None:
            BOOL.write(writer, True)
            INT_LE.write(writer, value.optional_block2)
        else:
            BOOL.write(writer, False)
        if value.optional_block3 is not None:
            BOOL.write(writer, True)
            INT_LE.write(writer, value.optional_block3)
        else:
            BOOL.write(writer, False)


_CAPPED_SURFACE = _BiomeCappedSurfaceDataType()

# ---------------------------------------------------------------------------
# BiomeConditionalTransformationData
# ---------------------------------------------------------------------------


@dataclass
class BiomeConditionalTransformationData:
    """A conditional block transformation rule."""

    weights: list[BiomeWeightedData] = field(default_factory=list)
    short1: int = 0
    int1: int = 0


class _BiomeConditionalTransformationDataType(Type["BiomeConditionalTransformationData"]):
    """BiomeWeightedData array + SHORT_LE + INT_LE."""

    def read(self, reader: PacketReader) -> BiomeConditionalTransformationData:
        count = UVAR_INT.read(reader)
        weights = [_WEIGHT.read(reader) for _ in range(count)]
        return BiomeConditionalTransformationData(
            weights=weights,
            short1=SHORT_LE.read(reader),
            int1=INT_LE.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeConditionalTransformationData) -> None:
        UVAR_INT.write(writer, len(value.weights))
        for w in value.weights:
            _WEIGHT.write(writer, w)
        SHORT_LE.write(writer, value.short1)
        INT_LE.write(writer, value.int1)


_CONDITIONAL_TRANSFORMATION = _BiomeConditionalTransformationDataType()

# ---------------------------------------------------------------------------
# BiomeOverworldGenRulesData
# ---------------------------------------------------------------------------


@dataclass
class BiomeOverworldGenRulesData:
    """Overworld biome generation rules.

    Attributes:
        weights1: First weight array.
        weights2: Second weight array.
        weights3: Third weight array.
        weights4: Fourth weight array.
        transformations1: First conditional transformation array.
        transformations2: Second conditional transformation array.
        weighted_temperatures: Weighted temperature array.
    """

    weights1: list[BiomeWeightedData] = field(default_factory=list)
    weights2: list[BiomeWeightedData] = field(default_factory=list)
    weights3: list[BiomeWeightedData] = field(default_factory=list)
    weights4: list[BiomeWeightedData] = field(default_factory=list)
    transformations1: list[BiomeConditionalTransformationData] = field(default_factory=list)
    transformations2: list[BiomeConditionalTransformationData] = field(default_factory=list)
    weighted_temperatures: list[BiomeWeightedTemperatureData] = field(default_factory=list)


class _BiomeOverworldGenRulesDataType(Type["BiomeOverworldGenRulesData"]):
    """4 weight arrays + 2 conditional transformation arrays + 1 weighted temperature array."""

    def _read_weights(self, reader: PacketReader) -> list[BiomeWeightedData]:
        count = UVAR_INT.read(reader)
        return [_WEIGHT.read(reader) for _ in range(count)]

    def _write_weights(self, writer: PacketWriter, weights: list[BiomeWeightedData]) -> None:
        UVAR_INT.write(writer, len(weights))
        for w in weights:
            _WEIGHT.write(writer, w)

    def _read_transformations(self, reader: PacketReader) -> list[BiomeConditionalTransformationData]:
        count = UVAR_INT.read(reader)
        return [_CONDITIONAL_TRANSFORMATION.read(reader) for _ in range(count)]

    def _write_transformations(
        self, writer: PacketWriter, transformations: list[BiomeConditionalTransformationData]
    ) -> None:
        UVAR_INT.write(writer, len(transformations))
        for t in transformations:
            _CONDITIONAL_TRANSFORMATION.write(writer, t)

    def read(self, reader: PacketReader) -> BiomeOverworldGenRulesData:
        return BiomeOverworldGenRulesData(
            weights1=self._read_weights(reader),
            weights2=self._read_weights(reader),
            weights3=self._read_weights(reader),
            weights4=self._read_weights(reader),
            transformations1=self._read_transformations(reader),
            transformations2=self._read_transformations(reader),
            weighted_temperatures=[_WEIGHTED_TEMPERATURE.read(reader) for _ in range(UVAR_INT.read(reader))],
        )

    def write(self, writer: PacketWriter, value: BiomeOverworldGenRulesData) -> None:
        self._write_weights(writer, value.weights1)
        self._write_weights(writer, value.weights2)
        self._write_weights(writer, value.weights3)
        self._write_weights(writer, value.weights4)
        self._write_transformations(writer, value.transformations1)
        self._write_transformations(writer, value.transformations2)
        UVAR_INT.write(writer, len(value.weighted_temperatures))
        for wt in value.weighted_temperatures:
            _WEIGHTED_TEMPERATURE.write(writer, wt)


_OVERWORLD_GEN_RULES = _BiomeOverworldGenRulesDataType()

# ---------------------------------------------------------------------------
# BiomeMultinoiseGenRulesData
# ---------------------------------------------------------------------------


@dataclass
class BiomeMultinoiseGenRulesData:
    """Multinoise biome generation rules."""

    float1: float
    float2: float
    float3: float
    float4: float
    float5: float


class _BiomeMultinoiseGenRulesDataType(Type["BiomeMultinoiseGenRulesData"]):
    """5 FLOAT_LE fields."""

    def read(self, reader: PacketReader) -> BiomeMultinoiseGenRulesData:
        return BiomeMultinoiseGenRulesData(
            float1=FLOAT_LE.read(reader),
            float2=FLOAT_LE.read(reader),
            float3=FLOAT_LE.read(reader),
            float4=FLOAT_LE.read(reader),
            float5=FLOAT_LE.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeMultinoiseGenRulesData) -> None:
        FLOAT_LE.write(writer, value.float1)
        FLOAT_LE.write(writer, value.float2)
        FLOAT_LE.write(writer, value.float3)
        FLOAT_LE.write(writer, value.float4)
        FLOAT_LE.write(writer, value.float5)


_MULTINOISE_GEN_RULES = _BiomeMultinoiseGenRulesDataType()

# ---------------------------------------------------------------------------
# BiomeLegacyWorldGenRulesData
# ---------------------------------------------------------------------------


@dataclass
class BiomeLegacyWorldGenRulesData:
    """Legacy world generation rules."""

    transformations: list[BiomeConditionalTransformationData] = field(default_factory=list)


class _BiomeLegacyWorldGenRulesDataType(Type["BiomeLegacyWorldGenRulesData"]):
    """Array of BiomeConditionalTransformationData."""

    def read(self, reader: PacketReader) -> BiomeLegacyWorldGenRulesData:
        count = UVAR_INT.read(reader)
        return BiomeLegacyWorldGenRulesData(
            transformations=[_CONDITIONAL_TRANSFORMATION.read(reader) for _ in range(count)],
        )

    def write(self, writer: PacketWriter, value: BiomeLegacyWorldGenRulesData) -> None:
        UVAR_INT.write(writer, len(value.transformations))
        for t in value.transformations:
            _CONDITIONAL_TRANSFORMATION.write(writer, t)


_LEGACY_WORLD_GEN_RULES = _BiomeLegacyWorldGenRulesDataType()

# ---------------------------------------------------------------------------
# BiomeReplacementData
# ---------------------------------------------------------------------------


@dataclass
class BiomeReplacementData:
    """Biome replacement data.

    Attributes:
        short1: First short parameter.
        short2: Second short parameter.
        shorts: Array of short values.
        float1: First float parameter.
        float2: Second float parameter.
        int1: Int parameter.
    """

    short1: int = 0
    short2: int = 0
    shorts: list[int] = field(default_factory=list)
    float1: float = 0.0
    float2: float = 0.0
    int1: int = 0


class _BiomeReplacementDataType(Type["BiomeReplacementData"]):
    """2 SHORT_LE + SHORT_LE array + 2 FLOAT_LE + INT_LE."""

    def read(self, reader: PacketReader) -> BiomeReplacementData:
        short1 = SHORT_LE.read(reader)
        short2 = SHORT_LE.read(reader)
        count = UVAR_INT.read(reader)
        shorts = [SHORT_LE.read(reader) for _ in range(count)]
        return BiomeReplacementData(
            short1=short1,
            short2=short2,
            shorts=shorts,
            float1=FLOAT_LE.read(reader),
            float2=FLOAT_LE.read(reader),
            int1=INT_LE.read(reader),
        )

    def write(self, writer: PacketWriter, value: BiomeReplacementData) -> None:
        SHORT_LE.write(writer, value.short1)
        SHORT_LE.write(writer, value.short2)
        UVAR_INT.write(writer, len(value.shorts))
        for s in value.shorts:
            SHORT_LE.write(writer, s)
        FLOAT_LE.write(writer, value.float1)
        FLOAT_LE.write(writer, value.float2)
        INT_LE.write(writer, value.int1)


_BIOME_REPLACEMENT_DATA = _BiomeReplacementDataType()

# ---------------------------------------------------------------------------
# BiomeDefinitionChunkGenData (version-specific)
# ---------------------------------------------------------------------------


@dataclass
class BiomeDefinitionChunkGenData:
    """Chunk generation data for a biome definition.

    All optional sub-structures are bool-prefixed. The village_type field
    only exists in v924 (BOOL prefix + BYTE value).

    Attributes:
        climate: Optional climate parameters.
        consolidated_features: Optional list of consolidated features.
        mountain_params: Optional mountain generation parameters.
        biome_elements: Optional list of biome elements.
        surface_material: Optional surface material.
        flag1: First boolean flag.
        flag2: Second boolean flag.
        flag3: Third boolean flag.
        flag4: Fourth boolean flag.
        mesa_surface: Optional mesa surface parameters.
        capped_surface: Optional capped surface parameters.
        overworld_gen_rules: Optional overworld generation rules.
        multinoise_gen_rules: Optional multinoise generation rules.
        legacy_world_gen_rules: Optional legacy world generation rules.
        biome_replacement_data: Optional biome replacement data.
        village_type: Optional village type (v924 only).
    """

    climate: BiomeClimateData | None = None
    consolidated_features: list[BiomeConsolidatedFeatureData] | None = None
    mountain_params: BiomeMountainParamsData | None = None
    biome_elements: list[BiomeElementData] | None = None
    surface_material: BiomeSurfaceMaterialData | None = None
    flag1: bool = False
    flag2: bool = False
    flag3: bool = False
    flag4: bool = False
    mesa_surface: BiomeMesaSurfaceData | None = None
    capped_surface: BiomeCappedSurfaceData | None = None
    overworld_gen_rules: BiomeOverworldGenRulesData | None = None
    multinoise_gen_rules: BiomeMultinoiseGenRulesData | None = None
    legacy_world_gen_rules: BiomeLegacyWorldGenRulesData | None = None
    biome_replacement_data: BiomeReplacementData | None = None
    village_type: int | None = None


class _BiomeDefinitionChunkGenDataV898Type(Type["BiomeDefinitionChunkGenData"]):
    """v898 BiomeDefinitionChunkGenData: no village_type field."""

    def _read_common(self, reader: PacketReader) -> BiomeDefinitionChunkGenData:
        """Read all fields shared between v898 and v924.

        Args:
            reader: The packet reader to read from.

        Returns:
            A BiomeDefinitionChunkGenData with all common fields populated.
        """
        # optional climate
        climate = _CLIMATE.read(reader) if BOOL.read(reader) else None
        # optional consolidated features array
        consolidated_features: list[BiomeConsolidatedFeatureData] | None = None
        if BOOL.read(reader):
            count = UVAR_INT.read(reader)
            consolidated_features = [_CONSOLIDATED_FEATURE.read(reader) for _ in range(count)]
        # optional mountain params
        mountain_params = _MOUNTAIN_PARAMS.read(reader) if BOOL.read(reader) else None
        # optional biome elements array
        biome_elements: list[BiomeElementData] | None = None
        if BOOL.read(reader):
            count = UVAR_INT.read(reader)
            biome_elements = [_BIOME_ELEMENT.read(reader) for _ in range(count)]
        # optional surface material
        surface_material = _SURFACE_MATERIAL.read(reader) if BOOL.read(reader) else None
        # 4 boolean flags
        flag1 = BOOL.read(reader)
        flag2 = BOOL.read(reader)
        flag3 = BOOL.read(reader)
        flag4 = BOOL.read(reader)
        # optional mesa surface
        mesa_surface = _MESA_SURFACE.read(reader) if BOOL.read(reader) else None
        # optional capped surface
        capped_surface = _CAPPED_SURFACE.read(reader) if BOOL.read(reader) else None
        # optional overworld gen rules
        overworld_gen_rules = _OVERWORLD_GEN_RULES.read(reader) if BOOL.read(reader) else None
        # optional multinoise gen rules
        multinoise_gen_rules = _MULTINOISE_GEN_RULES.read(reader) if BOOL.read(reader) else None
        # optional legacy world gen rules
        legacy_world_gen_rules = _LEGACY_WORLD_GEN_RULES.read(reader) if BOOL.read(reader) else None
        # optional biome replacement data
        biome_replacement_data = _BIOME_REPLACEMENT_DATA.read(reader) if BOOL.read(reader) else None
        return BiomeDefinitionChunkGenData(
            climate=climate,
            consolidated_features=consolidated_features,
            mountain_params=mountain_params,
            biome_elements=biome_elements,
            surface_material=surface_material,
            flag1=flag1,
            flag2=flag2,
            flag3=flag3,
            flag4=flag4,
            mesa_surface=mesa_surface,
            capped_surface=capped_surface,
            overworld_gen_rules=overworld_gen_rules,
            multinoise_gen_rules=multinoise_gen_rules,
            legacy_world_gen_rules=legacy_world_gen_rules,
            biome_replacement_data=biome_replacement_data,
        )

    def _write_common(self, writer: PacketWriter, value: BiomeDefinitionChunkGenData) -> None:
        """Write all fields shared between v898 and v924.

        Args:
            writer: The packet writer to write to.
            value: The BiomeDefinitionChunkGenData to serialize.
        """
        # optional climate
        if value.climate is not None:
            BOOL.write(writer, True)
            _CLIMATE.write(writer, value.climate)
        else:
            BOOL.write(writer, False)
        # optional consolidated features array
        if value.consolidated_features is not None:
            BOOL.write(writer, True)
            UVAR_INT.write(writer, len(value.consolidated_features))
            for cf in value.consolidated_features:
                _CONSOLIDATED_FEATURE.write(writer, cf)
        else:
            BOOL.write(writer, False)
        # optional mountain params
        if value.mountain_params is not None:
            BOOL.write(writer, True)
            _MOUNTAIN_PARAMS.write(writer, value.mountain_params)
        else:
            BOOL.write(writer, False)
        # optional biome elements array
        if value.biome_elements is not None:
            BOOL.write(writer, True)
            UVAR_INT.write(writer, len(value.biome_elements))
            for be in value.biome_elements:
                _BIOME_ELEMENT.write(writer, be)
        else:
            BOOL.write(writer, False)
        # optional surface material
        if value.surface_material is not None:
            BOOL.write(writer, True)
            _SURFACE_MATERIAL.write(writer, value.surface_material)
        else:
            BOOL.write(writer, False)
        # 4 boolean flags
        BOOL.write(writer, value.flag1)
        BOOL.write(writer, value.flag2)
        BOOL.write(writer, value.flag3)
        BOOL.write(writer, value.flag4)
        # optional mesa surface
        if value.mesa_surface is not None:
            BOOL.write(writer, True)
            _MESA_SURFACE.write(writer, value.mesa_surface)
        else:
            BOOL.write(writer, False)
        # optional capped surface
        if value.capped_surface is not None:
            BOOL.write(writer, True)
            _CAPPED_SURFACE.write(writer, value.capped_surface)
        else:
            BOOL.write(writer, False)
        # optional overworld gen rules
        if value.overworld_gen_rules is not None:
            BOOL.write(writer, True)
            _OVERWORLD_GEN_RULES.write(writer, value.overworld_gen_rules)
        else:
            BOOL.write(writer, False)
        # optional multinoise gen rules
        if value.multinoise_gen_rules is not None:
            BOOL.write(writer, True)
            _MULTINOISE_GEN_RULES.write(writer, value.multinoise_gen_rules)
        else:
            BOOL.write(writer, False)
        # optional legacy world gen rules
        if value.legacy_world_gen_rules is not None:
            BOOL.write(writer, True)
            _LEGACY_WORLD_GEN_RULES.write(writer, value.legacy_world_gen_rules)
        else:
            BOOL.write(writer, False)
        # optional biome replacement data
        if value.biome_replacement_data is not None:
            BOOL.write(writer, True)
            _BIOME_REPLACEMENT_DATA.write(writer, value.biome_replacement_data)
        else:
            BOOL.write(writer, False)

    def read(self, reader: PacketReader) -> BiomeDefinitionChunkGenData:
        return self._read_common(reader)

    def write(self, writer: PacketWriter, value: BiomeDefinitionChunkGenData) -> None:
        self._write_common(writer, value)


class _BiomeDefinitionChunkGenDataV924Type(_BiomeDefinitionChunkGenDataV898Type):
    """v924 BiomeDefinitionChunkGenData: common fields + trailing village_type (BOOL + BYTE)."""

    def read(self, reader: PacketReader) -> BiomeDefinitionChunkGenData:
        data = self._read_common(reader)
        # village_type (v924 only)
        if BOOL.read(reader):
            data.village_type = BYTE.read(reader)
        return data

    def write(self, writer: PacketWriter, value: BiomeDefinitionChunkGenData) -> None:
        self._write_common(writer, value)
        # village_type (v924 only)
        if value.village_type is not None:
            BOOL.write(writer, True)
            BYTE.write(writer, value.village_type)
        else:
            BOOL.write(writer, False)


# ---------------------------------------------------------------------------
# BiomeDefinitionData (version-specific)
# ---------------------------------------------------------------------------


@dataclass
class BiomeDefinitionData:
    """A single biome definition entry.

    Attributes:
        short1: Biome short identifier.
        float1: First float parameter.
        float2: Second float parameter.
        float3: Third float parameter.
        float4: Fourth float parameter.
        float5: Fifth float parameter.
        int1: Int parameter.
        flag: Boolean flag.
        tags: Optional list of tag IDs (USHORT_LE).
        chunk_gen_data: Optional chunk generation data.
    """

    short1: int = 0
    float1: float = 0.0
    float2: float = 0.0
    float3: float = 0.0
    float4: float = 0.0
    float5: float = 0.0
    int1: int = 0
    flag: bool = False
    tags: list[int] | None = None
    chunk_gen_data: BiomeDefinitionChunkGenData | None = None


class _BiomeDefinitionDataV898Type(Type["BiomeDefinitionData"]):
    """v898 BiomeDefinitionData: uses v898 BiomeDefinitionChunkGenData (no village_type)."""

    _chunk_gen_data_type: Type[BiomeDefinitionChunkGenData] = _BiomeDefinitionChunkGenDataV898Type()

    def read(self, reader: PacketReader) -> BiomeDefinitionData:
        short1 = SHORT_LE.read(reader)
        float1 = FLOAT_LE.read(reader)
        float2 = FLOAT_LE.read(reader)
        float3 = FLOAT_LE.read(reader)
        float4 = FLOAT_LE.read(reader)
        float5 = FLOAT_LE.read(reader)
        int1 = INT_LE.read(reader)
        flag = BOOL.read(reader)
        # optional tags array (BOOL prefix + UVAR_INT count + USHORT_LE[])
        tags: list[int] | None = None
        if BOOL.read(reader):
            count = UVAR_INT.read(reader)
            tags = [USHORT_LE.read(reader) for _ in range(count)]
        # optional chunk gen data
        chunk_gen_data = self._chunk_gen_data_type.read(reader) if BOOL.read(reader) else None
        return BiomeDefinitionData(
            short1=short1,
            float1=float1,
            float2=float2,
            float3=float3,
            float4=float4,
            float5=float5,
            int1=int1,
            flag=flag,
            tags=tags,
            chunk_gen_data=chunk_gen_data,
        )

    def write(self, writer: PacketWriter, value: BiomeDefinitionData) -> None:
        SHORT_LE.write(writer, value.short1)
        FLOAT_LE.write(writer, value.float1)
        FLOAT_LE.write(writer, value.float2)
        FLOAT_LE.write(writer, value.float3)
        FLOAT_LE.write(writer, value.float4)
        FLOAT_LE.write(writer, value.float5)
        INT_LE.write(writer, value.int1)
        BOOL.write(writer, value.flag)
        # optional tags array
        if value.tags is not None:
            BOOL.write(writer, True)
            UVAR_INT.write(writer, len(value.tags))
            for tag in value.tags:
                USHORT_LE.write(writer, tag)
        else:
            BOOL.write(writer, False)
        # optional chunk gen data
        if value.chunk_gen_data is not None:
            BOOL.write(writer, True)
            self._chunk_gen_data_type.write(writer, value.chunk_gen_data)
        else:
            BOOL.write(writer, False)


class _BiomeDefinitionDataV924Type(_BiomeDefinitionDataV898Type):
    """v924 BiomeDefinitionData: uses v924 BiomeDefinitionChunkGenData (with village_type)."""

    _chunk_gen_data_type: Type[BiomeDefinitionChunkGenData] = _BiomeDefinitionChunkGenDataV924Type()


# ---------------------------------------------------------------------------
# Singletons
# ---------------------------------------------------------------------------

BIOME_DEFINITION_V898 = _BiomeDefinitionDataV898Type()
BIOME_DEFINITION_V924 = _BiomeDefinitionDataV924Type()
