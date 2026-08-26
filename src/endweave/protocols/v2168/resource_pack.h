#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/resource_pack.h>

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

/** 2192 appended a fourth setting shape and left the three before it where they were, so every
 * value an older client can send is already the bytes 2192 reads. */
template <>
struct WireCompatible<bp::ServerboundPackSettingChangePacket_<2168>, bp::ServerboundPackSettingChangePacket_<2192>>
    : std::true_type {};

} // namespace endweave
