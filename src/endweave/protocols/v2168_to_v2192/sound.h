#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/sound.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::PlaySoundPacket_<2168>, bp::PlaySoundPacket_<2192>> {
    static void transform(Context<bp::PlaySoundPacket_<2192>> &ctx, bp::PlaySoundPacket_<2168> &&from);
};
} // namespace endweave
