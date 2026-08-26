#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/scoreboard.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ScoreboardIdentityPacketInfo_<1001>, bp::ScoreboardIdentityPacketInfo_<2168>> {
    static void transform(Context<bp::ScoreboardIdentityPacketInfo_<2168>> &ctx,
                          bp::ScoreboardIdentityPacketInfo_<1001> &&from);
};

template <>
struct Transformer<bp::SetScorePacket_<1001>, bp::SetScorePacket_<2168>> {
    static void transform(Context<bp::SetScorePacket_<2168>> &ctx, bp::SetScorePacket_<1001> &&from);
};

template <>
struct Transformer<bp::SetScoreboardIdentityPacket_<1001>, bp::SetScoreboardIdentityPacket_<2168>> {
    static void transform(Context<bp::SetScoreboardIdentityPacket_<2168>> &ctx,
                          bp::SetScoreboardIdentityPacket_<1001> &&from);
};

} // namespace endweave
