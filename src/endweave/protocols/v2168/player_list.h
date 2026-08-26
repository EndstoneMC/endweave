#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/player_list.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlayerListPacket_<2168>, bp::PlayerListPacket_<1001>> {
    static void transform(Context<bp::PlayerListPacket_<1001>> &ctx, bp::PlayerListPacket_<2168> &&from);
};

} // namespace endweave
