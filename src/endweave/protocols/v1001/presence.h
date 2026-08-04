#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/presence.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PresenceConfiguration_<1001>> {
    static bp::PresenceConfiguration_<2168> upgrade(bp::PresenceConfiguration_<1001> &&from);
};

template <>
struct Transformer<bp::GatheringsConfigurationJoinInfo_<1001>> {
    static bp::GatheringsConfigurationJoinInfo_<2168> upgrade(bp::GatheringsConfigurationJoinInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ServerConfigurationJoinInfo_<1001>> {
    static bp::ServerConfigurationJoinInfo_<2168> upgrade(bp::ServerConfigurationJoinInfo_<1001> &&from);
};

template <>
struct Transformer<bp::TransferPacket_<1001>> {
    static bp::TransferPacket_<2168> upgrade(bp::TransferPacket_<1001> &&from);
};

template <>
struct Transformer<bp::ServerPresenceInfoPacket_<1001>> {
    static bp::ServerPresenceInfoPacket_<2168> upgrade(bp::ServerPresenceInfoPacket_<1001> &&from);
};

} // namespace endweave
