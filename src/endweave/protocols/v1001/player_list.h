#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/player_list.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerListPacket_<1001>> {
    static bp::PlayerListPacket_<2168> upgrade(bp::PlayerListPacket_<1001> &&from);
};

} // namespace endweave
