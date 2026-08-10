#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/scoreboard.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ScoreboardIdentityPacketInfo_<2168>, bp::ScoreboardIdentityPacketInfo_<1001>> {
    static void transform(Context<bp::ScoreboardIdentityPacketInfo_<1001>> &ctx,
                          bp::ScoreboardIdentityPacketInfo_<2168> &&from);
};

template <>
struct Transformer<bp::SetScorePacket_<2168>, bp::SetScorePacket_<1001>> {
    static void transform(Context<bp::SetScorePacket_<1001>> &ctx, bp::SetScorePacket_<2168> &&from);
};

template <>
struct Transformer<bp::SetScoreboardIdentityPacket_<2168>, bp::SetScoreboardIdentityPacket_<1001>> {
    static void transform(Context<bp::SetScoreboardIdentityPacket_<1001>> &ctx,
                          bp::SetScoreboardIdentityPacket_<2168> &&from);
};

} // namespace endweave
