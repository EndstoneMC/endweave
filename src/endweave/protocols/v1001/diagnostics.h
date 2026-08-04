#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/diagnostics.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MemoryCategoryCounter_<1001>> {
    static bp::MemoryCategoryCounter_<2168> upgrade(bp::MemoryCategoryCounter_<1001> &&from);
};

template <>
struct Transformer<bp::ServerboundDiagnosticsPacket_<1001>> {
    static bp::ServerboundDiagnosticsPacket_<2168> upgrade(bp::ServerboundDiagnosticsPacket_<1001> &&from);
};

} // namespace endweave
