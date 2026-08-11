#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/movement.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MoveActorDeltaData_<2187>, bp::MoveActorDeltaData_<2168>> {
    static void transform(Context<bp::MoveActorDeltaData_<2168>> &ctx, bp::MoveActorDeltaData_<2187> &&from);
};

template <>
struct Transformer<bp::MoveActorDeltaPacket_<2187>, bp::MoveActorDeltaPacket_<2168>> {
    static void transform(Context<bp::MoveActorDeltaPacket_<2168>> &ctx, bp::MoveActorDeltaPacket_<2187> &&from);
};

} // namespace endweave
