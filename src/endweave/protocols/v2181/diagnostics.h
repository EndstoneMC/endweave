#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/diagnostics.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MemoryCategoryCounter_<2181>, bp::MemoryCategoryCounter_<2168>> {
    static bp::MemoryCategoryCounter_<2168> transform(bp::MemoryCategoryCounter_<2181> &&from);
};

template <>
struct Transformer<bp::EntityDiagnosticTimingInfo_<2181>, bp::EntityDiagnosticTimingInfo_<2168>> {
    static bp::EntityDiagnosticTimingInfo_<2168> transform(bp::EntityDiagnosticTimingInfo_<2181> &&from);
};

template <>
struct Transformer<bp::ServerboundDiagnosticsPacket_<2181>, bp::ServerboundDiagnosticsPacket_<2168>> {
    static bp::ServerboundDiagnosticsPacket_<2168> transform(bp::ServerboundDiagnosticsPacket_<2181> &&from);
};

} // namespace endweave
