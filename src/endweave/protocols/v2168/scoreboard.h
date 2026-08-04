#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/scoreboard.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ScoreboardIdentityPacketInfo_<2168>> {
    static bp::ScoreboardIdentityPacketInfo_<1001> downgrade(bp::ScoreboardIdentityPacketInfo_<2168> &&from);
};

template <>
struct Transformer<bp::SetScorePacket_<2168>> {
    static bp::SetScorePacket_<1001> downgrade(bp::SetScorePacket_<2168> &&from);
};

template <>
struct Transformer<bp::SetScoreboardIdentityPacket_<2168>> {
    static bp::SetScoreboardIdentityPacket_<1001> downgrade(bp::SetScoreboardIdentityPacket_<2168> &&from);
};

} // namespace endweave
