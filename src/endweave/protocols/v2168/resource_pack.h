#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/resource_pack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ResourcePacksInfoPacket_<2168>, bp::ResourcePacksInfoPacket_<1001>> {
    static void transform(Context<bp::ResourcePacksInfoPacket_<1001>> &ctx, bp::ResourcePacksInfoPacket_<2168> &&from);
};

template <>
struct Transformer<bp::ResourcePackClientResponsePacket_<2168>, bp::ResourcePackClientResponsePacket_<1001>> {
    static void transform(Context<bp::ResourcePackClientResponsePacket_<1001>> &ctx,
                          bp::ResourcePackClientResponsePacket_<2168> &&from);
};

} // namespace endweave
