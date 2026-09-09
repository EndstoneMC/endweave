#include "endweave/protocols/v2192_to_v2168/dimension.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::DimensionDefinitionGroup_<2192>::DimensionDefinition,
                 bp::DimensionDefinitionGroup_<2168>::DimensionDefinition>::
    transform(Context<bp::DimensionDefinitionGroup_<2168>::DimensionDefinition> &ctx,
              bp::DimensionDefinitionGroup_<2192>::DimensionDefinition &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: the floor and the span above it are the minimum and the maximum 2168 reads there.
    to.height_minimum = from.minimum_y;
    to.height_maximum = from.minimum_y + from.height_range;
    to.generator_type = from.generator_type;
    to.dimension_type = from.dimension_type;
    to.pack_id = from.pack_id;
    // ENDWEAVE: default_biome stops here. 2168 names no biome, and the generator picks as it always did.
}

} // namespace endweave
