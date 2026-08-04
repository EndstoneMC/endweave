#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/v1001/item_stack.h"

#include <protocol/input.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerBlockActionData_<1001>> {
    static bp::PlayerBlockActionData_<2168> upgrade(bp::PlayerBlockActionData_<1001> &&from);
};

template <>
struct Transformer<bp::PlayerAuthInputPacket_<1001>> {
    static bp::PlayerAuthInputPacket_<2168> upgrade(bp::PlayerAuthInputPacket_<1001> &&from);
};

} // namespace endweave
