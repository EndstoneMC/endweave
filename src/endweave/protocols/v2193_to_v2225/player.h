#pragma once

#include "endweave/protocol/transform.h"
#include "skin.h"

#include <bedrock/protocol/player.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::AddPlayerPacket_<2193>, bp::AddPlayerPacket_<2225>> {
    static void transform(Context<bp::AddPlayerPacket_<2225>> &ctx, bp::AddPlayerPacket_<2193> &&from);
};

template <>
struct Transformer<bp::PlayerListPacket_<2193>::AddEntry, bp::PlayerListPacket_<2225>::AddEntry> {
    static void transform(Context<bp::PlayerListPacket_<2225>::AddEntry> &ctx,
                          bp::PlayerListPacket_<2193>::AddEntry &&from);
};
} // namespace endweave
