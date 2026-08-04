#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/v1001/actor.h"
#include "endweave/protocols/v1001/inventory.h"

#include <cstdint>
#include <protocol/movement.h>

namespace bp = bedrock::protocol;

namespace endweave {

enum MoveActorDeltaHeader : std::uint16_t {
    CONTAINS_POSITION_X = 1 << 0,
    CONTAINS_POSITION_Y = 1 << 1,
    CONTAINS_POSITION_Z = 1 << 2,
    CONTAINS_ROTATION_X = 1 << 3,
    CONTAINS_ROTATION_Y = 1 << 4,
    CONTAINS_ROTATION_Y_HEAD = 1 << 5,
    IS_ON_GROUND = 1 << 6,
    FORCE_MOVE = 1 << 7,
    FORCE_MOVE_LOCAL_ENTITY = 1 << 8,
    FORCE_COMPLETION = 1 << 9,
};

template <>
struct Transformer<bp::MoveActorDeltaData_<1001>> {
    static bp::MoveActorDeltaData_<2168> upgrade(bp::MoveActorDeltaData_<1001> &&from);
};

template <>
struct Transformer<bp::AddPlayerPacket_<1001>> {
    static bp::AddPlayerPacket_<2168> upgrade(bp::AddPlayerPacket_<1001> &&from);
};

template <>
struct Transformer<bp::AddItemActorPacket_<1001>> {
    static bp::AddItemActorPacket_<2168> upgrade(bp::AddItemActorPacket_<1001> &&from);
};

template <>
struct Transformer<bp::MovePlayerPacket_<1001>> {
    static bp::MovePlayerPacket_<2168> upgrade(bp::MovePlayerPacket_<1001> &&from);
};

template <>
struct Transformer<bp::MoveActorDeltaPacket_<1001>> {
    static bp::MoveActorDeltaPacket_<2168> upgrade(bp::MoveActorDeltaPacket_<1001> &&from);
};

} // namespace endweave
