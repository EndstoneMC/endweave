#include "endweave/protocols/v2168_to_v2192/dimension.h"

namespace ew = endweave;

namespace endweave {
void Transformer<bp::DimensionDefinitionGroup_<2168>::DimensionDefinition,
                 bp::DimensionDefinitionGroup_<2192>::DimensionDefinition>::
    transform(Context<bp::DimensionDefinitionGroup_<2192>::DimensionDefinition> &ctx,
              bp::DimensionDefinitionGroup_<2168>::DimensionDefinition &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: 2192 keeps the two leading members in place and stops meaning by them what 2168 did:
    // a maximum and a minimum became a floor and the span above it.
    to.minimum_y = from.height_minimum;
    to.height_range = from.height_maximum - from.height_minimum;
    to.generator_type = from.generator_type;
    to.dimension_type = from.dimension_type;
    to.pack_id = from.pack_id;
    // ENDWEAVE: 2168 names no biome for a dimension, and empty is what 2192 writes for one that
    // takes whatever the generator gives it.
    to.default_biome = {};
}

} // namespace endweave
