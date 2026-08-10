#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/structure.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::StructureEditorData_<1001>, bp::StructureEditorData_<2168>> {
    static void transform(Context<bp::StructureEditorData_<2168>> &ctx, bp::StructureEditorData_<1001> &&from);
};

template <>
struct Transformer<bp::StructureBlockUpdatePacket_<1001>, bp::StructureBlockUpdatePacket_<2168>> {
    static void transform(Context<bp::StructureBlockUpdatePacket_<2168>> &ctx,
                          bp::StructureBlockUpdatePacket_<1001> &&from);
};

} // namespace endweave
