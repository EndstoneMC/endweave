#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/sound.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::ClientboundUpdateSoundDataPacket_<2193>, bp::ClientboundUpdateSoundDataPacket_<2225>> {
    static void transform(Context<bp::ClientboundUpdateSoundDataPacket_<2225>> &ctx,
                          bp::ClientboundUpdateSoundDataPacket_<2193> &&from);
};
} // namespace endweave
