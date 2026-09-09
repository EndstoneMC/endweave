#pragma once

#include "endweave/protocol/transform.h"
#include "inventory.h"

#include <bedrock/protocol/actor.h>
#include <bedrock/protocol/movement.h>
#include <bedrock/protocol/player.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::MoveActorDeltaData_<2168>, bp::MoveActorDeltaData_<2192>> {
    static void transform(Context<bp::MoveActorDeltaData_<2192>> &ctx, bp::MoveActorDeltaData_<2168> &&from);
};
} // namespace endweave
