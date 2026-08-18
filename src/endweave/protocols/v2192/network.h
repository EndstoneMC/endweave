#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/network.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DisconnectPacket_<2192>, bp::DisconnectPacket_<2168>> {
    static void transform(Context<bp::DisconnectPacket_<2168>> &ctx, bp::DisconnectPacket_<2192> &&from);
};

template <>
struct Transformer<bp::PacketViolationWarningPacket_<2192>, bp::PacketViolationWarningPacket_<2168>> {
    static void transform(Context<bp::PacketViolationWarningPacket_<2168>> &ctx,
                          bp::PacketViolationWarningPacket_<2192> &&from);
};

} // namespace endweave
