#include "attributes.h"

#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::EnvironmentAttributeData_<2193>, bp::EnvironmentAttributeData_<2225>>::transform(
    Context<bp::EnvironmentAttributeData_<2225>> &ctx, bp::EnvironmentAttributeData_<2193> &&from)
{
    auto &to = ctx.out();
    to.name = std::move(from.name);
    // ENDWEAVE: 2193 holds both ends only while a transition runs, so either one missing is a constant.
    if (!from.from_attribute.has_value() || !from.to_attribute.has_value()) {
        to.payload = bp::ConstantAttributeData_<2225>{.attribute = std::move(from.attribute)};
        return;
    }
    // ENDWEAVE: 2225 works the current value out from the two ends, so 2193's own copy is not sent.
    // 2193 names no clock for a transition to run on, so the clock is left unnamed.
    if (!from.noise_transition) {
        to.payload = bp::TransitionAttributeData_<2225>{
            .from_attribute = std::move(from.from_attribute).value(),
            .to_attribute = std::move(from.to_attribute).value(),
            .settings = {.total_transition_ticks = from.total_transition_ticks,
                         .current_transition_ticks = from.current_transition_ticks,
                         .easing = from.easing,
                         .clock_name = {}},
        };
        return;
    }
    // ENDWEAVE: 2193 names the noise once on the layer, which only AttributeLayerData can carry across.
    to.payload = bp::NoiseTransitionAttributeData_<2225>{
        .from_attribute = std::move(from.from_attribute).value(),
        .to_attribute = std::move(from.to_attribute).value(),
        .settings = {.total_transition_ticks = from.total_transition_ticks,
                     .current_transition_ticks = from.current_transition_ticks,
                     .easing = from.easing,
                     .clock_name = {},
                     .local_transition_ticks = from.local_transition_ticks,
                     .noise_name = {},
                     .noise_alignment = from.noise_alignment},
    };
}

void Transformer<bp::AttributeLayerData_<2193>, bp::AttributeLayerData_<2225>>::transform(
    Context<bp::AttributeLayerData_<2225>> &ctx, bp::AttributeLayerData_<2193> &&from)
{
    const auto noise_name = std::move(from.noise_name);
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    if (!noise_name.has_value()) {
        return;
    }
    for (auto &attribute : to.attributes) {
        if (auto *const noise = std::get_if<bp::NoiseTransitionAttributeData_<2225>>(&attribute.payload)) {
            noise->settings.noise_name = *noise_name;
        }
    }
}

} // namespace endweave
