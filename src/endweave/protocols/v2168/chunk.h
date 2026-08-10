#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/chunk.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::LevelChunkPacket_<2168>, bp::LevelChunkPacket_<1001>> {
    static void transform(Context<bp::LevelChunkPacket_<1001>> &ctx, bp::LevelChunkPacket_<2168> &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>::SubChunkPosOffset, bp::SubChunkPacket_<1001>::SubChunkPosOffset> {
    static void transform(Context<bp::SubChunkPacket_<1001>::SubChunkPosOffset> &ctx,
                          bp::SubChunkPacket_<2168>::SubChunkPosOffset &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>::HeightmapData, bp::SubChunkPacket_<1001>::HeightmapData> {
    static void transform(Context<bp::SubChunkPacket_<1001>::HeightmapData> &ctx,
                          bp::SubChunkPacket_<2168>::HeightmapData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>::SubChunkPacketData, bp::SubChunkPacket_<1001>::SubChunkPacketData> {
    static void transform(Context<bp::SubChunkPacket_<1001>::SubChunkPacketData> &ctx,
                          bp::SubChunkPacket_<2168>::SubChunkPacketData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>, bp::SubChunkPacket_<1001>> {
    static void transform(Context<bp::SubChunkPacket_<1001>> &ctx, bp::SubChunkPacket_<2168> &&from);
};

template <>
struct WireCompatible<bp::SubChunkRequestPacket_<2168>, bp::SubChunkRequestPacket_<1001>> : std::true_type {};

} // namespace endweave
