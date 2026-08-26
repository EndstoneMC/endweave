#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/presence.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PresenceConfiguration_<1001>, bp::PresenceConfiguration_<2168>> {
    static void transform(Context<bp::PresenceConfiguration_<2168>> &ctx, bp::PresenceConfiguration_<1001> &&from);
};

template <>
struct Transformer<bp::GatheringsConfigurationJoinInfo_<1001>, bp::GatheringsConfigurationJoinInfo_<2168>> {
    static void transform(Context<bp::GatheringsConfigurationJoinInfo_<2168>> &ctx,
                          bp::GatheringsConfigurationJoinInfo_<1001> &&from);
};

template <>
struct Transformer<bp::TransferPacket_<1001>, bp::TransferPacket_<2168>> {
    static void transform(Context<bp::TransferPacket_<2168>> &ctx, bp::TransferPacket_<1001> &&from);
};

} // namespace endweave
