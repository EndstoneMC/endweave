/// Biome definition codec implementation.

#include "endweave/codec/types/biome.h"

namespace endweave {

namespace {

enum class BiomeVersion { V898, V924 };

// -- Inner structure read/write helpers --

std::expected<BiomeClimateData, ReadError> read_climate(PacketReader& r) {
    BiomeClimateData d;
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.temperature = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.downfall = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.ash = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.red_spores = *v; }
    return d;
}

void write_climate(PacketWriter& w, const BiomeClimateData& d) {
    w.write_float_le(d.temperature);
    w.write_float_le(d.downfall);
    w.write_float_le(d.ash);
    w.write_float_le(d.red_spores);
}

std::expected<BiomeWeightedData, ReadError> read_weighted(PacketReader& r) {
    BiomeWeightedData d;
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.weight = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.block = *v; }
    return d;
}

void write_weighted(PacketWriter& w, const BiomeWeightedData& d) {
    w.write_short_le(d.weight);
    w.write_int_le(d.block);
}

std::expected<BiomeWeightedTemperatureData, ReadError> read_weighted_temp(PacketReader& r) {
    BiomeWeightedTemperatureData d;
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); d.temperature = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.weight = *v; }
    return d;
}

void write_weighted_temp(PacketWriter& w, const BiomeWeightedTemperatureData& d) {
    w.write_varint(d.temperature);
    w.write_int_le(d.weight);
}

std::expected<BiomeCoordinateData, ReadError> read_coordinate(PacketReader& r) {
    BiomeCoordinateData d;
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); d.expr_op1 = *v; }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short1 = *v; }
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); d.expr_op2 = *v; }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short2 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.int1 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.int2 = *v; }
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); d.varint = *v; }
    return d;
}

void write_coordinate(PacketWriter& w, const BiomeCoordinateData& d) {
    w.write_varint(d.expr_op1);
    w.write_short_le(d.short1);
    w.write_varint(d.expr_op2);
    w.write_short_le(d.short2);
    w.write_int_le(d.int1);
    w.write_int_le(d.int2);
    w.write_varint(d.varint);
}

std::expected<BiomeSurfaceMaterialData, ReadError> read_surface_material(PacketReader& r) {
    BiomeSurfaceMaterialData d;
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.block1 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.block2 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.block3 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.block4 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.block5 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.extra = *v; }
    return d;
}

void write_surface_material(PacketWriter& w, const BiomeSurfaceMaterialData& d) {
    w.write_int_le(d.block1);
    w.write_int_le(d.block2);
    w.write_int_le(d.block3);
    w.write_int_le(d.block4);
    w.write_int_le(d.block5);
    w.write_int_le(d.extra);
}

std::expected<BiomeElementData, ReadError> read_element(PacketReader& r) {
    BiomeElementData d;
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float1 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float2 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float3 = *v; }
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); d.expr_op1 = *v; }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short1 = *v; }
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); d.expr_op2 = *v; }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short2 = *v; }
    { auto v = read_surface_material(r); if (!v) return std::unexpected(v.error()); d.surface_material = *v; }
    return d;
}

void write_element(PacketWriter& w, const BiomeElementData& d) {
    w.write_float_le(d.float1);
    w.write_float_le(d.float2);
    w.write_float_le(d.float3);
    w.write_varint(d.expr_op1);
    w.write_short_le(d.short1);
    w.write_varint(d.expr_op2);
    w.write_short_le(d.short2);
    write_surface_material(w, d.surface_material);
}

std::expected<BiomeScatterParamData, ReadError> read_scatter_param(PacketReader& r) {
    BiomeScatterParamData d;
    { auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
      d.coordinates.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read_coordinate(r); if (!v) return std::unexpected(v.error());
          d.coordinates.push_back(*v);
      }
    }
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); d.varint1 = *v; }
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); d.varint2 = *v; }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short1 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.int1 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.int2 = *v; }
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); d.varint3 = *v; }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short2 = *v; }
    return d;
}

void write_scatter_param(PacketWriter& w, const BiomeScatterParamData& d) {
    w.write_uvarint(static_cast<std::uint32_t>(d.coordinates.size()));
    for (auto& c : d.coordinates) write_coordinate(w, c);
    w.write_varint(d.varint1);
    w.write_varint(d.varint2);
    w.write_short_le(d.short1);
    w.write_int_le(d.int1);
    w.write_int_le(d.int2);
    w.write_varint(d.varint3);
    w.write_short_le(d.short2);
}

std::expected<BiomeConsolidatedFeatureData, ReadError> read_consolidated_feature(PacketReader& r) {
    BiomeConsolidatedFeatureData d;
    { auto v = read_scatter_param(r); if (!v) return std::unexpected(v.error()); d.scatter_param = std::move(*v); }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short1 = *v; }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short2 = *v; }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short3 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag = *v; }
    return d;
}

void write_consolidated_feature(PacketWriter& w, const BiomeConsolidatedFeatureData& d) {
    write_scatter_param(w, d.scatter_param);
    w.write_short_le(d.short1);
    w.write_short_le(d.short2);
    w.write_short_le(d.short3);
    w.write_bool(d.flag);
}

std::expected<BiomeMountainParamsData, ReadError> read_mountain_params(PacketReader& r) {
    BiomeMountainParamsData d;
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.block = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag1 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag2 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag3 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag4 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag5 = *v; }
    return d;
}

void write_mountain_params(PacketWriter& w, const BiomeMountainParamsData& d) {
    w.write_int_le(d.block);
    w.write_bool(d.flag1);
    w.write_bool(d.flag2);
    w.write_bool(d.flag3);
    w.write_bool(d.flag4);
    w.write_bool(d.flag5);
}

std::expected<BiomeMesaSurfaceData, ReadError> read_mesa_surface(PacketReader& r) {
    BiomeMesaSurfaceData d;
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.block1 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.block2 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag1 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag2 = *v; }
    return d;
}

void write_mesa_surface(PacketWriter& w, const BiomeMesaSurfaceData& d) {
    w.write_int_le(d.block1);
    w.write_int_le(d.block2);
    w.write_bool(d.flag1);
    w.write_bool(d.flag2);
}

std::expected<BiomeCappedSurfaceData, ReadError> read_capped_surface(PacketReader& r) {
    BiomeCappedSurfaceData d;
    { auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
      d.blocks1.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = r.read_int_le(); if (!v) return std::unexpected(v.error());
          d.blocks1.push_back(*v);
      }
    }
    { auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
      d.blocks2.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = r.read_int_le(); if (!v) return std::unexpected(v.error());
          d.blocks2.push_back(*v);
      }
    }
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.optional_block1 = *v; }
    }
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.optional_block2 = *v; }
    }
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.optional_block3 = *v; }
    }
    return d;
}

void write_capped_surface(PacketWriter& w, const BiomeCappedSurfaceData& d) {
    w.write_uvarint(static_cast<std::uint32_t>(d.blocks1.size()));
    for (auto b : d.blocks1) w.write_int_le(b);
    w.write_uvarint(static_cast<std::uint32_t>(d.blocks2.size()));
    for (auto b : d.blocks2) w.write_int_le(b);
    if (d.optional_block1) { w.write_bool(true); w.write_int_le(*d.optional_block1); } else { w.write_bool(false); }
    if (d.optional_block2) { w.write_bool(true); w.write_int_le(*d.optional_block2); } else { w.write_bool(false); }
    if (d.optional_block3) { w.write_bool(true); w.write_int_le(*d.optional_block3); } else { w.write_bool(false); }
}

std::expected<BiomeConditionalTransformationData, ReadError> read_conditional_transformation(PacketReader& r) {
    BiomeConditionalTransformationData d;
    { auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
      d.weights.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read_weighted(r); if (!v) return std::unexpected(v.error());
          d.weights.push_back(*v);
      }
    }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short1 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.int1 = *v; }
    return d;
}

void write_conditional_transformation(PacketWriter& w, const BiomeConditionalTransformationData& d) {
    w.write_uvarint(static_cast<std::uint32_t>(d.weights.size()));
    for (auto& wt : d.weights) write_weighted(w, wt);
    w.write_short_le(d.short1);
    w.write_int_le(d.int1);
}

std::expected<std::vector<BiomeWeightedData>, ReadError> read_weight_array(PacketReader& r) {
    auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
    std::vector<BiomeWeightedData> result;
    result.reserve(*count);
    for (std::uint32_t i = 0; i < *count; ++i) {
        auto v = read_weighted(r); if (!v) return std::unexpected(v.error());
        result.push_back(*v);
    }
    return result;
}

void write_weight_array(PacketWriter& w, const std::vector<BiomeWeightedData>& arr) {
    w.write_uvarint(static_cast<std::uint32_t>(arr.size()));
    for (auto& wt : arr) write_weighted(w, wt);
}

std::expected<std::vector<BiomeConditionalTransformationData>, ReadError> read_transformation_array(PacketReader& r) {
    auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
    std::vector<BiomeConditionalTransformationData> result;
    result.reserve(*count);
    for (std::uint32_t i = 0; i < *count; ++i) {
        auto v = read_conditional_transformation(r); if (!v) return std::unexpected(v.error());
        result.push_back(std::move(*v));
    }
    return result;
}

void write_transformation_array(PacketWriter& w, const std::vector<BiomeConditionalTransformationData>& arr) {
    w.write_uvarint(static_cast<std::uint32_t>(arr.size()));
    for (auto& t : arr) write_conditional_transformation(w, t);
}

std::expected<BiomeOverworldGenRulesData, ReadError> read_overworld_gen_rules(PacketReader& r) {
    BiomeOverworldGenRulesData d;
    { auto v = read_weight_array(r); if (!v) return std::unexpected(v.error()); d.weights1 = std::move(*v); }
    { auto v = read_weight_array(r); if (!v) return std::unexpected(v.error()); d.weights2 = std::move(*v); }
    { auto v = read_weight_array(r); if (!v) return std::unexpected(v.error()); d.weights3 = std::move(*v); }
    { auto v = read_weight_array(r); if (!v) return std::unexpected(v.error()); d.weights4 = std::move(*v); }
    { auto v = read_transformation_array(r); if (!v) return std::unexpected(v.error()); d.transformations1 = std::move(*v); }
    { auto v = read_transformation_array(r); if (!v) return std::unexpected(v.error()); d.transformations2 = std::move(*v); }
    { auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
      d.weighted_temperatures.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = read_weighted_temp(r); if (!v) return std::unexpected(v.error());
          d.weighted_temperatures.push_back(*v);
      }
    }
    return d;
}

void write_overworld_gen_rules(PacketWriter& w, const BiomeOverworldGenRulesData& d) {
    write_weight_array(w, d.weights1);
    write_weight_array(w, d.weights2);
    write_weight_array(w, d.weights3);
    write_weight_array(w, d.weights4);
    write_transformation_array(w, d.transformations1);
    write_transformation_array(w, d.transformations2);
    w.write_uvarint(static_cast<std::uint32_t>(d.weighted_temperatures.size()));
    for (auto& wt : d.weighted_temperatures) write_weighted_temp(w, wt);
}

std::expected<BiomeMultinoiseGenRulesData, ReadError> read_multinoise_gen_rules(PacketReader& r) {
    BiomeMultinoiseGenRulesData d;
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float1 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float2 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float3 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float4 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float5 = *v; }
    return d;
}

void write_multinoise_gen_rules(PacketWriter& w, const BiomeMultinoiseGenRulesData& d) {
    w.write_float_le(d.float1);
    w.write_float_le(d.float2);
    w.write_float_le(d.float3);
    w.write_float_le(d.float4);
    w.write_float_le(d.float5);
}

std::expected<BiomeLegacyWorldGenRulesData, ReadError> read_legacy_world_gen_rules(PacketReader& r) {
    BiomeLegacyWorldGenRulesData d;
    auto v = read_transformation_array(r); if (!v) return std::unexpected(v.error());
    d.transformations = std::move(*v);
    return d;
}

void write_legacy_world_gen_rules(PacketWriter& w, const BiomeLegacyWorldGenRulesData& d) {
    write_transformation_array(w, d.transformations);
}

std::expected<BiomeReplacementData, ReadError> read_replacement(PacketReader& r) {
    BiomeReplacementData d;
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short1 = *v; }
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short2 = *v; }
    { auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
      d.shorts.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = r.read_short_le(); if (!v) return std::unexpected(v.error());
          d.shorts.push_back(*v);
      }
    }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float1 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float2 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.int1 = *v; }
    return d;
}

void write_replacement(PacketWriter& w, const BiomeReplacementData& d) {
    w.write_short_le(d.short1);
    w.write_short_le(d.short2);
    w.write_uvarint(static_cast<std::uint32_t>(d.shorts.size()));
    for (auto s : d.shorts) w.write_short_le(s);
    w.write_float_le(d.float1);
    w.write_float_le(d.float2);
    w.write_int_le(d.int1);
}

// -- BiomeDefinitionChunkGenData common read/write --

std::expected<BiomeDefinitionChunkGenData, ReadError> read_chunk_gen_common(PacketReader& r) {
    BiomeDefinitionChunkGenData d;

    // optional climate
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = read_climate(r); if (!v) return std::unexpected(v.error()); d.climate = *v; }
    }
    // optional consolidated features array
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) {
          auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
          std::vector<BiomeConsolidatedFeatureData> features;
          features.reserve(*count);
          for (std::uint32_t i = 0; i < *count; ++i) {
              auto v = read_consolidated_feature(r); if (!v) return std::unexpected(v.error());
              features.push_back(std::move(*v));
          }
          d.consolidated_features = std::move(features);
      }
    }
    // optional mountain params
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = read_mountain_params(r); if (!v) return std::unexpected(v.error()); d.mountain_params = *v; }
    }
    // optional biome elements array
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) {
          auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
          std::vector<BiomeElementData> elements;
          elements.reserve(*count);
          for (std::uint32_t i = 0; i < *count; ++i) {
              auto v = read_element(r); if (!v) return std::unexpected(v.error());
              elements.push_back(std::move(*v));
          }
          d.biome_elements = std::move(elements);
      }
    }
    // optional surface material
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = read_surface_material(r); if (!v) return std::unexpected(v.error()); d.surface_material = *v; }
    }
    // 4 boolean flags
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag1 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag2 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag3 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag4 = *v; }
    // optional mesa surface
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = read_mesa_surface(r); if (!v) return std::unexpected(v.error()); d.mesa_surface = *v; }
    }
    // optional capped surface
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = read_capped_surface(r); if (!v) return std::unexpected(v.error()); d.capped_surface = std::move(*v); }
    }
    // optional overworld gen rules
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = read_overworld_gen_rules(r); if (!v) return std::unexpected(v.error()); d.overworld_gen_rules = std::move(*v); }
    }
    // optional multinoise gen rules
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = read_multinoise_gen_rules(r); if (!v) return std::unexpected(v.error()); d.multinoise_gen_rules = *v; }
    }
    // optional legacy world gen rules
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = read_legacy_world_gen_rules(r); if (!v) return std::unexpected(v.error()); d.legacy_world_gen_rules = std::move(*v); }
    }
    // optional biome replacement data
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) { auto v = read_replacement(r); if (!v) return std::unexpected(v.error()); d.biome_replacement_data = std::move(*v); }
    }

    return d;
}

void write_chunk_gen_common(PacketWriter& w, const BiomeDefinitionChunkGenData& d) {
    if (d.climate) { w.write_bool(true); write_climate(w, *d.climate); } else { w.write_bool(false); }
    if (d.consolidated_features) {
        w.write_bool(true);
        w.write_uvarint(static_cast<std::uint32_t>(d.consolidated_features->size()));
        for (auto& cf : *d.consolidated_features) write_consolidated_feature(w, cf);
    } else { w.write_bool(false); }
    if (d.mountain_params) { w.write_bool(true); write_mountain_params(w, *d.mountain_params); } else { w.write_bool(false); }
    if (d.biome_elements) {
        w.write_bool(true);
        w.write_uvarint(static_cast<std::uint32_t>(d.biome_elements->size()));
        for (auto& be : *d.biome_elements) write_element(w, be);
    } else { w.write_bool(false); }
    if (d.surface_material) { w.write_bool(true); write_surface_material(w, *d.surface_material); } else { w.write_bool(false); }
    w.write_bool(d.flag1);
    w.write_bool(d.flag2);
    w.write_bool(d.flag3);
    w.write_bool(d.flag4);
    if (d.mesa_surface) { w.write_bool(true); write_mesa_surface(w, *d.mesa_surface); } else { w.write_bool(false); }
    if (d.capped_surface) { w.write_bool(true); write_capped_surface(w, *d.capped_surface); } else { w.write_bool(false); }
    if (d.overworld_gen_rules) { w.write_bool(true); write_overworld_gen_rules(w, *d.overworld_gen_rules); } else { w.write_bool(false); }
    if (d.multinoise_gen_rules) { w.write_bool(true); write_multinoise_gen_rules(w, *d.multinoise_gen_rules); } else { w.write_bool(false); }
    if (d.legacy_world_gen_rules) { w.write_bool(true); write_legacy_world_gen_rules(w, *d.legacy_world_gen_rules); } else { w.write_bool(false); }
    if (d.biome_replacement_data) { w.write_bool(true); write_replacement(w, *d.biome_replacement_data); } else { w.write_bool(false); }
}

// -- BiomeDefinitionData --

std::expected<BiomeDefinitionData, ReadError> read_biome_definition(PacketReader& r, BiomeVersion ver) {
    BiomeDefinitionData d;
    { auto v = r.read_short_le(); if (!v) return std::unexpected(v.error()); d.short1 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float1 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float2 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float3 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float4 = *v; }
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); d.float5 = *v; }
    { auto v = r.read_int_le(); if (!v) return std::unexpected(v.error()); d.int1 = *v; }
    { auto v = r.read_bool(); if (!v) return std::unexpected(v.error()); d.flag = *v; }

    // optional tags array (BOOL prefix + UVAR_INT count + USHORT_LE[])
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) {
          auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
          std::vector<std::uint16_t> tags;
          tags.reserve(*count);
          for (std::uint32_t i = 0; i < *count; ++i) {
              auto v = r.read_ushort_le(); if (!v) return std::unexpected(v.error());
              tags.push_back(*v);
          }
          d.tags = std::move(tags);
      }
    }

    // optional chunk gen data
    { auto has = r.read_bool(); if (!has) return std::unexpected(has.error());
      if (*has) {
          auto cg = read_chunk_gen_common(r);
          if (!cg) return std::unexpected(cg.error());
          if (ver == BiomeVersion::V924) {
              // village_type (v924 only)
              auto has_vt = r.read_bool(); if (!has_vt) return std::unexpected(has_vt.error());
              if (*has_vt) {
                  auto vt = r.read_byte(); if (!vt) return std::unexpected(vt.error());
                  cg->village_type = *vt;
              }
          }
          d.chunk_gen_data = std::move(*cg);
      }
    }

    return d;
}

void write_biome_definition(PacketWriter& w, const BiomeDefinitionData& d, BiomeVersion ver) {
    w.write_short_le(d.short1);
    w.write_float_le(d.float1);
    w.write_float_le(d.float2);
    w.write_float_le(d.float3);
    w.write_float_le(d.float4);
    w.write_float_le(d.float5);
    w.write_int_le(d.int1);
    w.write_bool(d.flag);

    // optional tags array
    if (d.tags) {
        w.write_bool(true);
        w.write_uvarint(static_cast<std::uint32_t>(d.tags->size()));
        for (auto t : *d.tags) w.write_ushort_le(t);
    } else {
        w.write_bool(false);
    }

    // optional chunk gen data
    if (d.chunk_gen_data) {
        w.write_bool(true);
        write_chunk_gen_common(w, *d.chunk_gen_data);
        if (ver == BiomeVersion::V924) {
            // village_type (v924 only)
            if (d.chunk_gen_data->village_type) {
                w.write_bool(true);
                w.write_byte(*d.chunk_gen_data->village_type);
            } else {
                w.write_bool(false);
            }
        }
    } else {
        w.write_bool(false);
    }
}

} // namespace

// -- Template specializations --

template <> auto PacketReader::read<biome_definition_v898>() -> std::expected<BiomeDefinitionData, ReadError> {
    return read_biome_definition(*this, BiomeVersion::V898);
}
template <> void PacketWriter::write<biome_definition_v898>(const BiomeDefinitionData& val) {
    write_biome_definition(*this, val, BiomeVersion::V898);
}
template <> auto PacketReader::read<biome_definition_v924>() -> std::expected<BiomeDefinitionData, ReadError> {
    return read_biome_definition(*this, BiomeVersion::V924);
}
template <> void PacketWriter::write<biome_definition_v924>(const BiomeDefinitionData& val) {
    write_biome_definition(*this, val, BiomeVersion::V924);
}

} // namespace endweave
