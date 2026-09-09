#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/chunk.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::SubChunkPacket_<2168>::HeightmapData, bp::SubChunkPacket_<2192>::HeightmapData> {
    static void transform(Context<bp::SubChunkPacket_<2192>::HeightmapData> &ctx,
                          bp::SubChunkPacket_<2168>::HeightmapData &&from);
};
} // namespace endweave
