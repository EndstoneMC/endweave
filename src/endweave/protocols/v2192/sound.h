#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/sound.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PlaySoundPacket_<2192>, bp::PlaySoundPacket_<2168>> {
    static void transform(Context<bp::PlaySoundPacket_<2168>> &ctx, bp::PlaySoundPacket_<2192> &&from);
};

} // namespace endweave
