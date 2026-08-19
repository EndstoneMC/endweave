#include "endweave/protocols/v2168/dimension.h"

namespace ew = endweave;

namespace endweave {
namespace {

using DimensionDefinitionV1001 = bp::DimensionDefinitionGroup_<1001>::DimensionDefinition;
using DimensionDefinitionV2168 = bp::DimensionDefinitionGroup_<2168>::DimensionDefinition;

} // namespace

void Transformer<DimensionDefinitionV2168, DimensionDefinitionV1001>::transform(Context<DimensionDefinitionV1001> &ctx,
                                                                                DimensionDefinitionV2168 &&from)
{
    DimensionDefinitionV1001 to;
    to.height_maximum = from.height_maximum;
    to.height_minimum = from.height_minimum;
    to.generator_type = from.generator_type;
    to.dimension_type = from.dimension_type;
    // ENDWEAVE: 1001 has nowhere to put the owning pack, so pack_id is dropped and the name key stands alone.
}

} // namespace endweave
