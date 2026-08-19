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
    static void transform(Context<bp::MoveActorDeltaData_<1001>> &ctx, bp::MoveActorDeltaData_<2168> &&from);
};

template <>
struct Transformer<bp::AddPlayerPacket_<2168>, bp::AddPlayerPacket_<1001>> {
    static void transform(Context<bp::AddPlayerPacket_<1001>> &ctx, bp::AddPlayerPacket_<2168> &&from);
};

template <>
struct Transformer<bp::AddItemActorPacket_<2168>, bp::AddItemActorPacket_<1001>> {
    static void transform(Context<bp::AddItemActorPacket_<1001>> &ctx, bp::AddItemActorPacket_<2168> &&from);
};

template <>
struct Transformer<bp::MovePlayerPacket_<2168>, bp::MovePlayerPacket_<1001>> {
    static void transform(Context<bp::MovePlayerPacket_<1001>> &ctx, bp::MovePlayerPacket_<2168> &&from);
};

template <>
struct Transformer<bp::MoveActorDeltaData_<2168>, bp::MoveActorDeltaData_<2192>> {
    static void transform(Context<bp::MoveActorDeltaData_<2192>> &ctx, bp::MoveActorDeltaData_<2168> &&from);
};

} // namespace endweave
