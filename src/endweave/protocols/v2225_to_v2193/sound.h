#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/sound.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::ClientboundUpdateSoundDataPacket_<2225>, bp::ClientboundUpdateSoundDataPacket_<2193>> {
    static void transform(Context<bp::ClientboundUpdateSoundDataPacket_<2193>> &ctx,
                          bp::ClientboundUpdateSoundDataPacket_<2225> &&from);
};
} // namespace endweave
