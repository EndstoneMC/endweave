#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/structure.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::StructureEditorData_<1001>> {
    static bp::StructureEditorData_<2168> upgrade(bp::StructureEditorData_<1001> &&from);
};

template <>
struct Transformer<bp::StructureBlockUpdatePacket_<1001>> {
    static bp::StructureBlockUpdatePacket_<2168> upgrade(bp::StructureBlockUpdatePacket_<1001> &&from);
};

} // namespace endweave
