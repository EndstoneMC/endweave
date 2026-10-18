#pragma once

#include "endweave/protocol/transform.h"
#include "skin.h"

#include <bedrock/protocol/player.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::PlayerListPacket_<2225>::AddEntry, bp::PlayerListPacket_<2193>::AddEntry> {
    static void transform(Context<bp::PlayerListPacket_<2193>::AddEntry> &ctx,
                          bp::PlayerListPacket_<2225>::AddEntry &&from);
};
} // namespace endweave
