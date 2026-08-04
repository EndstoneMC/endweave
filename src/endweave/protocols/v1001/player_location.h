#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/player_location.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerUpdateEntityOverridesPacket_<1001>> {
    static bp::PlayerUpdateEntityOverridesPacket_<2168> upgrade(bp::PlayerUpdateEntityOverridesPacket_<1001> &&from);
};

template <>
struct Transformer<bp::PlayerLocationPacket_<1001>> {
    static bp::PlayerLocationPacket_<2168> upgrade(bp::PlayerLocationPacket_<1001> &&from);
};

} // namespace endweave
