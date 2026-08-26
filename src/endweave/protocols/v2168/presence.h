#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/presence.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PresenceConfiguration_<2168>, bp::PresenceConfiguration_<1001>> {
    static void transform(Context<bp::PresenceConfiguration_<1001>> &ctx, bp::PresenceConfiguration_<2168> &&from);
};

template <>
struct Transformer<bp::GatheringsConfigurationJoinInfo_<2168>, bp::GatheringsConfigurationJoinInfo_<1001>> {
    static void transform(Context<bp::GatheringsConfigurationJoinInfo_<1001>> &ctx,
                          bp::GatheringsConfigurationJoinInfo_<2168> &&from);
};

} // namespace endweave
