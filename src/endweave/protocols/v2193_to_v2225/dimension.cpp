#include "dimension.h"

#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::DimensionDataPacket_<2193>::DimensionDefinition,
                 bp::DimensionDataPacket_<2225>::DimensionDefinition>::
    transform(Context<bp::DimensionDataPacket_<2225>::DimensionDefinition> &ctx,
              bp::DimensionDataPacket_<2193>::DimensionDefinition &&from)
{
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    // ENDWEAVE: 2193 sends no clouds component, and a dimension without one draws its clouds at 128.
    to.cloud_height = 128;
    to.render_clouds = true;
}

} // namespace endweave
