#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/resource_pack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ResourcePackClientResponsePacket_<1001>, bp::ResourcePackClientResponsePacket_<2168>> {
    static void transform(Context<bp::ResourcePackClientResponsePacket_<2168>> &ctx,
                          bp::ResourcePackClientResponsePacket_<1001> &&from);
};

} // namespace endweave
