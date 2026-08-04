#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/scoreboard.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ScoreboardIdentityPacketInfo_<1001>> {
    static bp::ScoreboardIdentityPacketInfo_<2168> upgrade(bp::ScoreboardIdentityPacketInfo_<1001> &&from);
};

template <>
struct Transformer<bp::SetScorePacket_<1001>> {
    static bp::SetScorePacket_<2168> upgrade(bp::SetScorePacket_<1001> &&from);
};

template <>
struct Transformer<bp::SetScoreboardIdentityPacket_<1001>> {
    static bp::SetScoreboardIdentityPacket_<2168> upgrade(bp::SetScoreboardIdentityPacket_<1001> &&from);
};

} // namespace endweave
