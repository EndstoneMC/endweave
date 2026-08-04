#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/v2168/item_stack.h"

#include <protocol/input.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerBlockActionData_<2168>> {
    static bp::PlayerBlockActionData_<1001> downgrade(bp::PlayerBlockActionData_<2168> &&from);
};

template <>
struct Transformer<bp::PlayerAuthInputPacket_<2168>> {
    static bp::PlayerAuthInputPacket_<1001> downgrade(bp::PlayerAuthInputPacket_<2168> &&from);
};

} // namespace endweave
