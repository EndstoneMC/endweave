#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/player_location.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerUpdateEntityOverridesPacket_<2168>> {
    static bp::PlayerUpdateEntityOverridesPacket_<1001> downgrade(bp::PlayerUpdateEntityOverridesPacket_<2168> &&from);
};

template <>
struct Transformer<bp::PlayerLocationPacket_<2168>> {
    static bp::PlayerLocationPacket_<1001> downgrade(bp::PlayerLocationPacket_<2168> &&from);
};

} // namespace endweave
