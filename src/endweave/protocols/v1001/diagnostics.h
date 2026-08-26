#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/diagnostics.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MemoryCategoryCounter_<1001>, bp::MemoryCategoryCounter_<2168>> {
    static void transform(Context<bp::MemoryCategoryCounter_<2168>> &ctx, bp::MemoryCategoryCounter_<1001> &&from);
};

template <>
struct Transformer<bp::ServerboundDiagnosticsPacket_<1001>, bp::ServerboundDiagnosticsPacket_<2168>> {
    static void transform(Context<bp::ServerboundDiagnosticsPacket_<2168>> &ctx,
                          bp::ServerboundDiagnosticsPacket_<1001> &&from);
};

} // namespace endweave
