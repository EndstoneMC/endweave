#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/movement.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MoveActorDeltaData_<2181>, bp::MoveActorDeltaData_<2168>> {
    static bp::MoveActorDeltaData_<2168> transform(bp::MoveActorDeltaData_<2181> &&from);
};

template <>
struct Transformer<bp::MoveActorDeltaPacket_<2181>, bp::MoveActorDeltaPacket_<2168>> {
    static bp::MoveActorDeltaPacket_<2168> transform(bp::MoveActorDeltaPacket_<2181> &&from);
};

} // namespace endweave
