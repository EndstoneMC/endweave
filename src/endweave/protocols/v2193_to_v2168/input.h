#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/enums.h"
#include "inventory.h"

#include <bedrock/protocol/input.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerAuthInputPacket_<2193>, bp::PlayerAuthInputPacket_<2168>> {
    static void transform(Context<bp::PlayerAuthInputPacket_<2168>> &ctx, bp::PlayerAuthInputPacket_<2193> &&from);
};

} // namespace endweave
