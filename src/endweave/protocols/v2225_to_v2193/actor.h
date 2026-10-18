#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/actor.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::AnimatePacket_<2225>, bp::AnimatePacket_<2193>> {
    static void transform(Context<bp::AnimatePacket_<2193>> &ctx, bp::AnimatePacket_<2225> &&from);
};
} // namespace endweave
