#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/resource_pack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ResourcePacksInfoPacket_<1001>> {
    static bp::ResourcePacksInfoPacket_<2168> upgrade(bp::ResourcePacksInfoPacket_<1001> &&from);
};

template <>
struct Transformer<bp::ResourcePackClientResponsePacket_<1001>> {
    static bp::ResourcePackClientResponsePacket_<2168> upgrade(bp::ResourcePackClientResponsePacket_<1001> &&from);
};

} // namespace endweave
