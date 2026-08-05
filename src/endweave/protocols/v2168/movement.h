#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/v1001/movement.h"
#include "endweave/protocols/v2168/actor.h"
#include "endweave/protocols/v2168/inventory.h"

#include <protocol/movement.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MoveActorDeltaData_<2168>, bp::MoveActorDeltaData_<1001>> {
    static bp::MoveActorDeltaData_<1001> transform(bp::MoveActorDeltaData_<2168> &&from);
};

template <>
struct Transformer<bp::AddPlayerPacket_<2168>, bp::AddPlayerPacket_<1001>> {
    static bp::AddPlayerPacket_<1001> transform(bp::AddPlayerPacket_<2168> &&from);
};

template <>
struct Transformer<bp::AddItemActorPacket_<2168>, bp::AddItemActorPacket_<1001>> {
    static bp::AddItemActorPacket_<1001> transform(bp::AddItemActorPacket_<2168> &&from);
};

template <>
struct Transformer<bp::MovePlayerPacket_<2168>, bp::MovePlayerPacket_<1001>> {
    static bp::MovePlayerPacket_<1001> transform(bp::MovePlayerPacket_<2168> &&from);
};

template <>
struct Transformer<bp::MoveActorDeltaPacket_<2168>, bp::MoveActorDeltaPacket_<1001>> {
    static bp::MoveActorDeltaPacket_<1001> transform(bp::MoveActorDeltaPacket_<2168> &&from);
};

} // namespace endweave
