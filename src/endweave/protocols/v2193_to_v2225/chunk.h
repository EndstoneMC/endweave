#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/chunk.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::LevelChunkPacket_<2193>, bp::LevelChunkPacket_<2225>> {
    static void transform(Context<bp::LevelChunkPacket_<2225>> &ctx, bp::LevelChunkPacket_<2193> &&from);
};
} // namespace endweave
