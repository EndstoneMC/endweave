#include "endweave/protocols/v1001/dimension.h"

namespace ew = endweave;

namespace endweave {
namespace {

using DimensionDefinitionV1001 = bp::DimensionDefinitionGroup_<1001>::DimensionDefinition;
using DimensionDefinitionV2168 = bp::DimensionDefinitionGroup_<2168>::DimensionDefinition;

} // namespace

void Transformer<DimensionDefinitionV1001, DimensionDefinitionV2168>::transform(Context<DimensionDefinitionV2168> &ctx,
                                                                                DimensionDefinitionV1001 &&from)
{
    DimensionDefinitionV2168 to;
    to.height_maximum = from.height_maximum;
    to.height_minimum = from.height_minimum;
    to.generator_type = from.generator_type;
    to.dimension_type = from.dimension_type;
    // ENDWEAVE: 1001 names no owning pack, and the null UUID is what BDS leaves when there is none.
    to.pack_id = {};
}

void Transformer<bp::DimensionDataPacket_<1001>, bp::DimensionDataPacket_<2168>>::transform(
    Context<bp::DimensionDataPacket_<2168>> &ctx, bp::DimensionDataPacket_<1001> &&from)
{
    auto &to = ctx.out();
    to.dimension_definitions = ew::transform(ctx, std::move(from.dimension_definitions));
}

} // namespace endweave
