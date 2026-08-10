#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/resource_pack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ResourcePacksInfoPacket_<1001>, bp::ResourcePacksInfoPacket_<2168>> {
    static void transform(Context<bp::ResourcePacksInfoPacket_<2168>> &ctx, bp::ResourcePacksInfoPacket_<1001> &&from);
};

template <>
struct Transformer<bp::ResourcePackClientResponsePacket_<1001>, bp::ResourcePackClientResponsePacket_<2168>> {
    static void transform(Context<bp::ResourcePackClientResponsePacket_<2168>> &ctx,
                          bp::ResourcePackClientResponsePacket_<1001> &&from);
};

} // namespace endweave
