#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/player_list.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerListPacket_<2168>> {
    static bp::PlayerListPacket_<1001> downgrade(bp::PlayerListPacket_<2168> &&from);
};

} // namespace endweave
