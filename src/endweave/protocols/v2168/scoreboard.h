#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/scoreboard.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ScoreboardIdentityPacketInfo_<2168>, bp::ScoreboardIdentityPacketInfo_<1001>> {
    static bp::ScoreboardIdentityPacketInfo_<1001> transform(bp::ScoreboardIdentityPacketInfo_<2168> &&from);
};

template <>
struct Transformer<bp::SetScorePacket_<2168>, bp::SetScorePacket_<1001>> {
    static bp::SetScorePacket_<1001> transform(bp::SetScorePacket_<2168> &&from);
};

template <>
struct Transformer<bp::SetScoreboardIdentityPacket_<2168>, bp::SetScoreboardIdentityPacket_<1001>> {
    static bp::SetScoreboardIdentityPacket_<1001> transform(bp::SetScoreboardIdentityPacket_<2168> &&from);
};

} // namespace endweave
