#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/player_list.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerListPacket_<1001>, bp::PlayerListPacket_<2168>> {
    static void transform(Context<bp::PlayerListPacket_<2168>> &ctx, bp::PlayerListPacket_<1001> &&from);
};

} // namespace endweave
