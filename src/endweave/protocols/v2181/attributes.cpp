#include "endweave/protocols/v2181/attributes.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

bp::EnvironmentAttributeData_<2168> Transformer<
    bp::EnvironmentAttributeData_<2181>,
    bp::EnvironmentAttributeData_<2168>>::transform(bp::EnvironmentAttributeData_<2181> &&from)
{
    bp::EnvironmentAttributeData_<2168> to;
    to.name = std::move(from.name);
    to.from_attribute = std::move(from.from_attribute);
    to.attribute = std::move(from.attribute);
    to.to_attribute = std::move(from.to_attribute);
    to.current_transition_ticks = from.current_transition_ticks;
    to.total_transition_ticks = from.total_transition_ticks;
    to.easing = from.easing;
    to.local_transition_ticks = from.local_transition_ticks;
    // ENDWEAVE: noise_alignment is dropped; 2168 aligns a noise transition by the local tick alone.
    to.noise_transition = from.noise_transition;
    return to;
}

bp::AttributeLayerData_<2168> Transformer<bp::AttributeLayerData_<2181>, bp::AttributeLayerData_<2168>>::transform(
    bp::AttributeLayerData_<2181> &&from)
{
    bp::AttributeLayerData_<2168> to;
    to.name = std::move(from.name);
    to.noise_name = std::move(from.noise_name);
    to.dimension_id = from.dimension_id;
    to.settings = from.settings;
    to.attributes = ew::transform(std::move(from.attributes));
    return to;
}

bp::UpdateAttributeLayersData_<2168> Transformer<
    bp::UpdateAttributeLayersData_<2181>,
    bp::UpdateAttributeLayersData_<2168>>::transform(bp::UpdateAttributeLayersData_<2181> &&from)
{
    bp::UpdateAttributeLayersData_<2168> to;
    to.attribute_layers = ew::transform(std::move(from.attribute_layers));
    return to;
}

bp::UpdateEnvironmentAttributesData_<2168> Transformer<
    bp::UpdateEnvironmentAttributesData_<2181>,
    bp::UpdateEnvironmentAttributesData_<2168>>::transform(bp::UpdateEnvironmentAttributesData_<2181> &&from)
{
    bp::UpdateEnvironmentAttributesData_<2168> to;
    to.layer_name = std::move(from.layer_name);
    to.layer_dimension_id = from.layer_dimension_id;
    to.attributes = ew::transform(std::move(from.attributes));
    return to;
}

bp::ClientboundAttributeLayerSyncPacket_<2168> Transformer<
    bp::ClientboundAttributeLayerSyncPacket_<2181>,
    bp::ClientboundAttributeLayerSyncPacket_<2168>>::transform(bp::ClientboundAttributeLayerSyncPacket_<2181> &&from)
{
    bp::ClientboundAttributeLayerSyncPacket_<2168> to;
    // ENDWEAVE: the settings and removal arms are one type at both versions; only the two carrying
    // attribute data moved, and the assignment places each back in its own case.
    std::visit(
        [&to](auto &alt) {
            using Alt = std::remove_cvref_t<decltype(alt)>;
            if constexpr (std::is_same_v<Alt, bp::UpdateAttributeLayersData_<2181>>) {
                to.data = ew::transform_to<bp::UpdateAttributeLayersData_<2168>>(std::move(alt));
            }
            else if constexpr (std::is_same_v<Alt, bp::UpdateEnvironmentAttributesData_<2181>>) {
                to.data = ew::transform_to<bp::UpdateEnvironmentAttributesData_<2168>>(std::move(alt));
            }
            else {
                to.data = std::move(alt);
            }
        },
        from.data);
    return to;
}

} // namespace endweave
