#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/diagnostics.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MemoryCategoryCounter_<2168>, bp::MemoryCategoryCounter_<1001>> {
    static void transform(Context<bp::MemoryCategoryCounter_<1001>> &ctx, bp::MemoryCategoryCounter_<2168> &&from);
};

template <>
struct Transformer<bp::ServerboundDiagnosticsPacket_<2168>, bp::ServerboundDiagnosticsPacket_<1001>> {
    static void transform(Context<bp::ServerboundDiagnosticsPacket_<1001>> &ctx,
                          bp::ServerboundDiagnosticsPacket_<2168> &&from);
};

template <>
struct Transformer<bp::MemoryCategoryCounter_<2168>, bp::MemoryCategoryCounter_<2181>> {
    static void transform(Context<bp::MemoryCategoryCounter_<2181>> &ctx, bp::MemoryCategoryCounter_<2168> &&from);
};

template <>
struct Transformer<bp::EntityDiagnosticTimingInfo_<2168>, bp::EntityDiagnosticTimingInfo_<2181>> {
    static void transform(Context<bp::EntityDiagnosticTimingInfo_<2181>> &ctx,
                          bp::EntityDiagnosticTimingInfo_<2168> &&from);
};

template <>
struct Transformer<bp::ServerboundDiagnosticsPacket_<2168>, bp::ServerboundDiagnosticsPacket_<2181>> {
    static void transform(Context<bp::ServerboundDiagnosticsPacket_<2181>> &ctx,
                          bp::ServerboundDiagnosticsPacket_<2168> &&from);
};

} // namespace endweave
