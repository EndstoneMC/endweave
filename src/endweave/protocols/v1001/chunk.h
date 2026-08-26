#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/chunk.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::LevelChunkPacket_<1001>, bp::LevelChunkPacket_<2168>> {
    static void transform(Context<bp::LevelChunkPacket_<2168>> &ctx, bp::LevelChunkPacket_<1001> &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::HeightmapData, bp::SubChunkPacket_<2168>::HeightmapData> {
    static void transform(Context<bp::SubChunkPacket_<2168>::HeightmapData> &ctx,
                          bp::SubChunkPacket_<1001>::HeightmapData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::SubChunkPacketData, bp::SubChunkPacket_<2168>::SubChunkPacketData> {
    static void transform(Context<bp::SubChunkPacket_<2168>::SubChunkPacketData> &ctx,
                          bp::SubChunkPacket_<1001>::SubChunkPacketData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::UncachedSubChunkPacketData,
                   bp::SubChunkPacket_<2168>::SubChunkPacketData> {
    static void transform(Context<bp::SubChunkPacket_<2168>::SubChunkPacketData> &ctx,
                          bp::SubChunkPacket_<1001>::UncachedSubChunkPacketData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>, bp::SubChunkPacket_<2168>> {
    static void transform(Context<bp::SubChunkPacket_<2168>> &ctx, bp::SubChunkPacket_<1001> &&from);
};

// ENDWEAVE: only the offsets' enclosing packet changed shape; SubChunkPosOffset is three int8_t at
// both versions and center_pos is 1001's SubChunkPos in both, so the request is already the bytes
// the other side expects.
} // namespace endweave
