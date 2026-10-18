#include "diagnostics.h"

#include <optional>
#include <utility>

namespace endweave {
void Transformer<bp::EntityDiagnosticTimingInfo_<2168>, bp::EntityDiagnosticTimingInfo_<2193>>::transform(
    Context<bp::EntityDiagnosticTimingInfo_<2193>> &ctx, bp::EntityDiagnosticTimingInfo_<2168> &&from)
{
    auto &to = ctx.out();
    to.display_name = std::move(from.display_name);
    to.entity = std::move(from.entity);
    to.time_in_ns = from.time_in_ns;
    to.percent_of_total = from.percent_of_total;
    // ENDWEAVE: 2168 locates the timed entity by nothing but its name, and absent is what 2193 writes
    // when the client has no position to offer.
    to.position = std::nullopt;
    to.dimension = std::nullopt;
}

} // namespace endweave
