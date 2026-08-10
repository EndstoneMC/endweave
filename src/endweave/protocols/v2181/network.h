#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/network.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DisconnectPacket_<2181>, bp::DisconnectPacket_<2168>> {
    static bp::DisconnectPacket_<2168> transform(bp::DisconnectPacket_<2181> &&from);
};

template <>
struct Transformer<bp::PacketViolationWarningPacket_<2181>, bp::PacketViolationWarningPacket_<2168>> {
    static bp::PacketViolationWarningPacket_<2168> transform(bp::PacketViolationWarningPacket_<2181> &&from);
};

} // namespace endweave
