#pragma once

/// Biome generation compound types for BiomeDefinitionList packet translation.
///
/// All inner structures are shared between v898 and v924. The only difference is
/// BiomeDefinitionChunkGenData: v924 appends a trailing village_type field
/// (BOOL + BYTE) that v898 does not have.

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

#include "endweave/codec/reader.h"
#include "endweave/codec/types/primitives.h"
#include "endweave/codec/writer.h"

namespace endweave {

// -- Nested structure declarations (implementation detail) --

struct BiomeClimateData {
    float temperature = 0.0f;
    float downfall = 0.0f;
    float ash = 0.0f;
    float red_spores = 0.0f;
};

struct BiomeWeightedData {
    std::int16_t weight = 0;
    std::int32_t block = 0;
};

struct BiomeWeightedTemperatureData {
    std::int32_t temperature = 0;
    std::int32_t weight = 0;
};

struct BiomeCoordinateData {
    std::int32_t expr_op1 = 0;
    std::int16_t short1 = 0;
    std::int32_t expr_op2 = 0;
    std::int16_t short2 = 0;
    std::int32_t int1 = 0;
    std::int32_t int2 = 0;
    std::int32_t varint = 0;
};

struct BiomeSurfaceMaterialData {
    std::int32_t block1 = 0;
    std::int32_t block2 = 0;
    std::int32_t block3 = 0;
    std::int32_t block4 = 0;
    std::int32_t block5 = 0;
    std::int32_t extra = 0;
};

struct BiomeElementData {
    float float1 = 0.0f;
    float float2 = 0.0f;
    float float3 = 0.0f;
    std::int32_t expr_op1 = 0;
    std::int16_t short1 = 0;
    std::int32_t expr_op2 = 0;
    std::int16_t short2 = 0;
    BiomeSurfaceMaterialData surface_material;
};

struct BiomeScatterParamData {
    std::vector<BiomeCoordinateData> coordinates;
    std::int32_t varint1 = 0;
    std::int32_t varint2 = 0;
    std::int16_t short1 = 0;
    std::int32_t int1 = 0;
    std::int32_t int2 = 0;
    std::int32_t varint3 = 0;
    std::int16_t short2 = 0;
};

struct BiomeConsolidatedFeatureData {
    BiomeScatterParamData scatter_param;
    std::int16_t short1 = 0;
    std::int16_t short2 = 0;
    std::int16_t short3 = 0;
    bool flag = false;
};

struct BiomeMountainParamsData {
    std::int32_t block = 0;
    bool flag1 = false;
    bool flag2 = false;
    bool flag3 = false;
    bool flag4 = false;
    bool flag5 = false;
};

struct BiomeMesaSurfaceData {
    std::int32_t block1 = 0;
    std::int32_t block2 = 0;
    bool flag1 = false;
    bool flag2 = false;
};

struct BiomeCappedSurfaceData {
    std::vector<std::int32_t> blocks1;
    std::vector<std::int32_t> blocks2;
    std::optional<std::int32_t> optional_block1;
    std::optional<std::int32_t> optional_block2;
    std::optional<std::int32_t> optional_block3;
};

struct BiomeConditionalTransformationData {
    std::vector<BiomeWeightedData> weights;
    std::int16_t short1 = 0;
    std::int32_t int1 = 0;
};

struct BiomeOverworldGenRulesData {
    std::vector<BiomeWeightedData> weights1;
    std::vector<BiomeWeightedData> weights2;
    std::vector<BiomeWeightedData> weights3;
    std::vector<BiomeWeightedData> weights4;
    std::vector<BiomeConditionalTransformationData> transformations1;
    std::vector<BiomeConditionalTransformationData> transformations2;
    std::vector<BiomeWeightedTemperatureData> weighted_temperatures;
};

struct BiomeMultinoiseGenRulesData {
    float float1 = 0.0f;
    float float2 = 0.0f;
    float float3 = 0.0f;
    float float4 = 0.0f;
    float float5 = 0.0f;
};

struct BiomeLegacyWorldGenRulesData {
    std::vector<BiomeConditionalTransformationData> transformations;
};

struct BiomeReplacementData {
    std::int16_t short1 = 0;
    std::int16_t short2 = 0;
    std::vector<std::int16_t> shorts;
    float float1 = 0.0f;
    float float2 = 0.0f;
    std::int32_t int1 = 0;
};

struct BiomeDefinitionChunkGenData {
    std::optional<BiomeClimateData> climate;
    std::optional<std::vector<BiomeConsolidatedFeatureData>> consolidated_features;
    std::optional<BiomeMountainParamsData> mountain_params;
    std::optional<std::vector<BiomeElementData>> biome_elements;
    std::optional<BiomeSurfaceMaterialData> surface_material;
    bool flag1 = false;
    bool flag2 = false;
    bool flag3 = false;
    bool flag4 = false;
    std::optional<BiomeMesaSurfaceData> mesa_surface;
    std::optional<BiomeCappedSurfaceData> capped_surface;
    std::optional<BiomeOverworldGenRulesData> overworld_gen_rules;
    std::optional<BiomeMultinoiseGenRulesData> multinoise_gen_rules;
    std::optional<BiomeLegacyWorldGenRulesData> legacy_world_gen_rules;
    std::optional<BiomeReplacementData> biome_replacement_data;
    std::optional<std::uint8_t> village_type; // v924 only
};

struct BiomeDefinitionData {
    std::int16_t short1 = 0;
    float float1 = 0.0f;
    float float2 = 0.0f;
    float float3 = 0.0f;
    float float4 = 0.0f;
    float float5 = 0.0f;
    std::int32_t int1 = 0;
    bool flag = false;
    std::optional<std::vector<std::uint16_t>> tags;
    std::optional<BiomeDefinitionChunkGenData> chunk_gen_data;
};

// -- Public type tags --

struct biome_definition_v898 { using value_type = BiomeDefinitionData; };
struct biome_definition_v924 { using value_type = BiomeDefinitionData; };

template <> auto PacketReader::read<biome_definition_v898>() -> std::expected<BiomeDefinitionData, ReadError>;
template <> void PacketWriter::write<biome_definition_v898>(const BiomeDefinitionData& val);
template <> auto PacketReader::read<biome_definition_v924>() -> std::expected<BiomeDefinitionData, ReadError>;
template <> void PacketWriter::write<biome_definition_v924>(const BiomeDefinitionData& val);

} // namespace endweave
