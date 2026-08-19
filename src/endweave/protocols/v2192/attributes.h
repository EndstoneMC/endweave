#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/attributes.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ClientboundAttributeLayerSyncPacket_<2192>, bp::ClientboundAttributeLayerSyncPacket_<2168>> {
    static void transform(Context<bp::ClientboundAttributeLayerSyncPacket_<2168>> &ctx,
                          bp::ClientboundAttributeLayerSyncPacket_<2192> &&from);
};

} // namespace endweave
