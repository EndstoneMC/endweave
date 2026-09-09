#include "endweave/protocols/v2192_to_v2168/attributes.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::ClientboundAttributeLayerSyncPacket_<2192>, bp::ClientboundAttributeLayerSyncPacket_<2168>>::
    transform(Context<bp::ClientboundAttributeLayerSyncPacket_<2168>> &ctx,
              bp::ClientboundAttributeLayerSyncPacket_<2192> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: the settings and removal arms are one type at both versions; only the two carrying
    // attribute data moved, and the assignment places each back in its own case.
    std::visit(
        [&to, &ctx](auto &alt) {
            using Alt = std::remove_cvref_t<decltype(alt)>;
            if constexpr (std::is_same_v<Alt, bp::UpdateAttributeLayersData_<2192>>) {
                to.data = ew::transform_to<bp::UpdateAttributeLayersData_<2168>>(ctx, std::move(alt));
            }
            else if constexpr (std::is_same_v<Alt, bp::UpdateEnvironmentAttributesData_<2192>>) {
                to.data = ew::transform_to<bp::UpdateEnvironmentAttributesData_<2168>>(ctx, std::move(alt));
            }
            else {
                to.data = std::move(alt);
            }
        },
        from.data);
}

} // namespace endweave
