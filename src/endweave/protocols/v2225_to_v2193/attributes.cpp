#include "attributes.h"

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::EnvironmentAttributeData_<2225>, bp::EnvironmentAttributeData_<2193>>::transform(
    Context<bp::EnvironmentAttributeData_<2193>> &ctx, bp::EnvironmentAttributeData_<2225> &&from)
{
    auto &to = ctx.out();
    to.name = std::move(from.name);
    std::visit(
        [&to]<class Payload>(Payload &&payload) {
            if constexpr (std::is_same_v<Payload, bp::ConstantAttributeData_<2225>>) {
                to.from_attribute = std::nullopt;
                to.attribute = std::move(payload.attribute);
                to.to_attribute = std::nullopt;
                to.current_transition_ticks = 0;
                to.total_transition_ticks = 0;
                to.easing = bp::EasingType::Linear;
                to.local_transition_ticks = 0;
                to.noise_transition = false;
                to.noise_alignment = {};
            }
            else {
                constexpr bool noise = std::is_same_v<Payload, bp::NoiseTransitionAttributeData_<2225>>;
                // ENDWEAVE: 2225 sends no current value for a running transition, so 2193 is handed the
                // end it starts from. 2193 has nowhere to name the clock it runs on, which is dropped.
                to.from_attribute = payload.from_attribute;
                to.attribute = std::move(payload.from_attribute);
                to.to_attribute = std::move(payload.to_attribute);
                to.current_transition_ticks = payload.settings.current_transition_ticks;
                to.total_transition_ticks = payload.settings.total_transition_ticks;
                to.easing = payload.settings.easing;
                if constexpr (noise) {
                    to.local_transition_ticks = payload.settings.local_transition_ticks;
                    to.noise_transition = true;
                    to.noise_alignment = payload.settings.noise_alignment;
                }
                else {
                    to.local_transition_ticks = 0;
                    to.noise_transition = false;
                    to.noise_alignment = {};
                }
            }
        },
        std::move(from.payload));
}

void Transformer<bp::AttributeLayerData_<2225>, bp::AttributeLayerData_<2193>>::transform(
    Context<bp::AttributeLayerData_<2193>> &ctx, bp::AttributeLayerData_<2225> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: 2193 names one noise for the whole layer, so the layer takes the first noise transition's
    // and any attribute naming another runs on it instead.
    to.noise_name = std::nullopt;
    for (const auto &attribute : from.attributes) {
        if (const auto *const noise = std::get_if<bp::NoiseTransitionAttributeData_<2225>>(&attribute.payload)) {
            to.noise_name = noise->settings.noise_name;
            break;
        }
    }
    ew::transform_members(ctx, std::move(from), to);
}

} // namespace endweave
