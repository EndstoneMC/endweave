#include "attributes.h"

#include <utility>

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

} // namespace endweave
