#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/network.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DisconnectPacket_<2168>, bp::DisconnectPacket_<2192>> {
    static void transform(Context<bp::DisconnectPacket_<2192>> &ctx, bp::DisconnectPacket_<2168> &&from);
};

template <>
struct Transformer<bp::PacketViolationWarningPacket_<2168>, bp::PacketViolationWarningPacket_<2192>> {
    static void transform(Context<bp::PacketViolationWarningPacket_<2192>> &ctx,
                          bp::PacketViolationWarningPacket_<2168> &&from);
};

} // namespace endweave
