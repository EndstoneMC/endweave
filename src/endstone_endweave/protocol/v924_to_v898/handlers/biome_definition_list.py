"""BiomeDefinitionList handler for v924 to v898."""

from collections.abc import Callable

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    FLOAT_LE,
    INT_LE,
    SHORT_LE,
    STRING,
    USHORT_LE,
    UVAR_INT,
    VAR_INT,
    PacketWrapper,
)


def _passthrough_array(wrapper: PacketWrapper, item_reader: Callable[[PacketWrapper], object]) -> None:
    count = wrapper.passthrough(UVAR_INT)
    for _ in range(count):
        item_reader(wrapper)


def _passthrough_optional(wrapper: PacketWrapper, item_reader: Callable[[PacketWrapper], object]) -> None:
    if wrapper.passthrough(BOOL):
        item_reader(wrapper)


def _passthrough_block(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(INT_LE)


def _passthrough_expression_op(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(VAR_INT)


def _passthrough_climate(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)


def _passthrough_coordinate(wrapper: PacketWrapper) -> None:
    _passthrough_expression_op(wrapper)
    wrapper.passthrough(SHORT_LE)
    _passthrough_expression_op(wrapper)
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(VAR_INT)


def _passthrough_scatter_param(wrapper: PacketWrapper) -> None:
    _passthrough_array(wrapper, _passthrough_coordinate)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(SHORT_LE)


def _passthrough_consolidated_feature(wrapper: PacketWrapper) -> None:
    _passthrough_scatter_param(wrapper)
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(BOOL)


def _passthrough_mountain_params(wrapper: PacketWrapper) -> None:
    _passthrough_block(wrapper)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)


def _passthrough_surface_material(wrapper: PacketWrapper) -> None:
    _passthrough_block(wrapper)
    _passthrough_block(wrapper)
    _passthrough_block(wrapper)
    _passthrough_block(wrapper)
    _passthrough_block(wrapper)
    wrapper.passthrough(INT_LE)


def _passthrough_biome_element(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    _passthrough_expression_op(wrapper)
    wrapper.passthrough(SHORT_LE)
    _passthrough_expression_op(wrapper)
    wrapper.passthrough(SHORT_LE)
    _passthrough_surface_material(wrapper)


def _passthrough_mesa_surface(wrapper: PacketWrapper) -> None:
    _passthrough_block(wrapper)
    _passthrough_block(wrapper)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)


def _passthrough_capped_surface(wrapper: PacketWrapper) -> None:
    _passthrough_array(wrapper, _passthrough_block)
    _passthrough_array(wrapper, _passthrough_block)
    _passthrough_optional(wrapper, _passthrough_block)
    _passthrough_optional(wrapper, _passthrough_block)
    _passthrough_optional(wrapper, _passthrough_block)


def _passthrough_weight(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(INT_LE)


def _passthrough_conditional_transformation(wrapper: PacketWrapper) -> None:
    _passthrough_array(wrapper, _passthrough_weight)
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(INT_LE)


def _passthrough_weighted_temperature(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(INT_LE)


def _passthrough_overworld_gen_rules(wrapper: PacketWrapper) -> None:
    _passthrough_array(wrapper, _passthrough_weight)
    _passthrough_array(wrapper, _passthrough_weight)
    _passthrough_array(wrapper, _passthrough_weight)
    _passthrough_array(wrapper, _passthrough_weight)
    _passthrough_array(wrapper, _passthrough_conditional_transformation)
    _passthrough_array(wrapper, _passthrough_conditional_transformation)
    _passthrough_array(wrapper, _passthrough_weighted_temperature)


def _passthrough_multinoise_gen_rules(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)


def _passthrough_legacy_world_gen_rules(wrapper: PacketWrapper) -> None:
    _passthrough_array(wrapper, _passthrough_conditional_transformation)


def _passthrough_biome_replacement_data(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(SHORT_LE)
    _passthrough_array(wrapper, lambda inner: inner.passthrough(SHORT_LE))
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(INT_LE)


def _passthrough_chunk_gen_data(wrapper: PacketWrapper) -> None:
    _passthrough_optional(wrapper, _passthrough_climate)
    _passthrough_optional(wrapper, lambda inner: _passthrough_array(inner, _passthrough_consolidated_feature))
    _passthrough_optional(wrapper, _passthrough_mountain_params)
    _passthrough_optional(wrapper, lambda inner: _passthrough_array(inner, _passthrough_biome_element))
    _passthrough_optional(wrapper, _passthrough_surface_material)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    _passthrough_optional(wrapper, _passthrough_mesa_surface)
    _passthrough_optional(wrapper, _passthrough_capped_surface)
    _passthrough_optional(wrapper, _passthrough_overworld_gen_rules)
    _passthrough_optional(wrapper, _passthrough_multinoise_gen_rules)
    _passthrough_optional(wrapper, _passthrough_legacy_world_gen_rules)
    _passthrough_optional(wrapper, _passthrough_biome_replacement_data)

    has_village_type = wrapper.read(BOOL)
    if has_village_type:
        wrapper.read(BYTE)


def _passthrough_definition(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(BOOL)

    if wrapper.passthrough(BOOL):
        tag_count = wrapper.passthrough(UVAR_INT)
        for _ in range(tag_count):
            wrapper.passthrough(USHORT_LE)

    _passthrough_optional(wrapper, _passthrough_chunk_gen_data)


def rewrite_biome_definition_list(wrapper: PacketWrapper) -> None:
    """Strip the v924 villageType field from biome chunk-gen data.

    Args:
        wrapper: Packet wrapper for BiomeDefinitionList.
    """
    biome_count = wrapper.passthrough(UVAR_INT)
    for _ in range(biome_count):
        wrapper.passthrough(USHORT_LE)
        _passthrough_definition(wrapper)

    string_count = wrapper.passthrough(UVAR_INT)
    for _ in range(string_count):
        wrapper.passthrough(STRING)
