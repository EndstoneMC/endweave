#include "endweave/protocols/v1001/dimension.h"

namespace ew = endweave;

namespace endweave {
namespace {

using DimensionDefinitionV1001 = bp::DimensionDefinitionGroup_<1001>::DimensionDefinition;
using DimensionDefinitionV2168 = bp::DimensionDefinitionGroup_<2168>::DimensionDefinition;

} // namespace

DimensionDefinitionV2168 Transformer<DimensionDefinitionV1001, DimensionDefinitionV2168>::transform(
    DimensionDefinitionV1001 &&from)
{
    DimensionDefinitionV2168 to;
    to.height_maximum = from.height_maximum;
    to.height_minimum = from.height_minimum;
    to.generator_type = from.generator_type;
    to.dimension_type = from.dimension_type;
    // ENDWEAVE: 1001 names no owning pack, and the null UUID is what BDS leaves when there is none.
    to.pack_id = {};
    return to;
}

bp::DimensionDataPacket_<2168> Transformer<bp::DimensionDataPacket_<1001>, bp::DimensionDataPacket_<2168>>::transform(
    bp::DimensionDataPacket_<1001> &&from)
{
    bp::DimensionDataPacket_<2168> to;
    to.dimension_definitions = ew::transform(std::move(from.dimension_definitions));
    return to;
}

} // namespace endweave
