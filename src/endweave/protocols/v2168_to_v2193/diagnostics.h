#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/diagnostics.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::EntityDiagnosticTimingInfo_<2168>, bp::EntityDiagnosticTimingInfo_<2193>> {
    static void transform(Context<bp::EntityDiagnosticTimingInfo_<2193>> &ctx,
                          bp::EntityDiagnosticTimingInfo_<2168> &&from);
};
} // namespace endweave
