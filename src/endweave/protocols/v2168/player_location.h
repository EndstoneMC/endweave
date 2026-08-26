#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/player_location.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerUpdateEntityOverridesPacket_<2168>, bp::PlayerUpdateEntityOverridesPacket_<1001>> {
    static void transform(Context<bp::PlayerUpdateEntityOverridesPacket_<1001>> &ctx,
                          bp::PlayerUpdateEntityOverridesPacket_<2168> &&from);
};

template <>
struct Transformer<bp::PlayerLocationPacket_<2168>, bp::PlayerLocationPacket_<1001>> {
    static void transform(Context<bp::PlayerLocationPacket_<1001>> &ctx, bp::PlayerLocationPacket_<2168> &&from);
};

} // namespace endweave
