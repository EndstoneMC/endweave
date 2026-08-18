#include "endweave/protocols/v2168/attributes.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::EnvironmentAttributeData_<2168>, bp::EnvironmentAttributeData_<2192>>::transform(
    Context<bp::EnvironmentAttributeData_<2192>> &ctx, bp::EnvironmentAttributeData_<2168> &&from)
{
    auto &to = ctx.out();
    to.name = std::move(from.name);
    to.from_attribute = std::move(from.from_attribute);
    to.attribute = std::move(from.attribute);
    to.to_attribute = std::move(from.to_attribute);
    to.current_transition_ticks = from.current_transition_ticks;
    to.total_transition_ticks = from.total_transition_ticks;
    to.easing = from.easing;
    to.local_transition_ticks = from.local_transition_ticks;
    to.noise_transition = from.noise_transition;
    // ENDWEAVE: 2168 sends no alignment, and NoiseAlignmentType has the one member, so the default pair
    // is the only thing 2192 could be told.
    to.noise_alignment = {};
}

void Transformer<bp::AttributeLayerData_<2168>, bp::AttributeLayerData_<2192>>::transform(
    Context<bp::AttributeLayerData_<2192>> &ctx, bp::AttributeLayerData_<2168> &&from)
{
    auto &to = ctx.out();
    to.name = std::move(from.name);
    to.noise_name = std::move(from.noise_name);
    to.dimension_id = from.dimension_id;
    to.settings = from.settings;
    to.attributes = ew::transform(ctx, std::move(from.attributes));
}

void Transformer<bp::UpdateAttributeLayersData_<2168>, bp::UpdateAttributeLayersData_<2192>>::transform(
    Context<bp::UpdateAttributeLayersData_<2192>> &ctx, bp::UpdateAttributeLayersData_<2168> &&from)
{
    auto &to = ctx.out();
    to.attribute_layers = ew::transform(ctx, std::move(from.attribute_layers));
}

void Transformer<bp::UpdateEnvironmentAttributesData_<2168>, bp::UpdateEnvironmentAttributesData_<2192>>::transform(
    Context<bp::UpdateEnvironmentAttributesData_<2192>> &ctx, bp::UpdateEnvironmentAttributesData_<2168> &&from)
{
    auto &to = ctx.out();
    to.layer_name = std::move(from.layer_name);
    to.layer_dimension_id = from.layer_dimension_id;
    to.attributes = ew::transform(ctx, std::move(from.attributes));
}

void Transformer<bp::ClientboundAttributeLayerSyncPacket_<2168>, bp::ClientboundAttributeLayerSyncPacket_<2192>>::
    transform(Context<bp::ClientboundAttributeLayerSyncPacket_<2192>> &ctx,
              bp::ClientboundAttributeLayerSyncPacket_<2168> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: the settings and removal arms are one type at both versions; only the two carrying
    // attribute data moved, and the assignment places each back in its own case.
    std::visit(
        [&to, &ctx](auto &alt) {
            using Alt = std::remove_cvref_t<decltype(alt)>;
            if constexpr (std::is_same_v<Alt, bp::UpdateAttributeLayersData_<2168>>) {
                to.data = ew::transform_to<bp::UpdateAttributeLayersData_<2192>>(ctx, std::move(alt));
            }
            else if constexpr (std::is_same_v<Alt, bp::UpdateEnvironmentAttributesData_<2168>>) {
                to.data = ew::transform_to<bp::UpdateEnvironmentAttributesData_<2192>>(ctx, std::move(alt));
            }
            else {
                to.data = std::move(alt);
            }
        },
        from.data);
}

} // namespace endweave
