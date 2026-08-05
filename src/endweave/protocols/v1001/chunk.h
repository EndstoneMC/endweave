#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/chunk.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::LevelChunkPacket_<1001>, bp::LevelChunkPacket_<2168>> {
    static bp::LevelChunkPacket_<2168> transform(bp::LevelChunkPacket_<1001> &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::SubChunkPosOffset, bp::SubChunkPacket_<2168>::SubChunkPosOffset> {
    static bp::SubChunkPacket_<2168>::SubChunkPosOffset transform(bp::SubChunkPacket_<1001>::SubChunkPosOffset &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::HeightmapData, bp::SubChunkPacket_<2168>::HeightmapData> {
    static bp::SubChunkPacket_<2168>::HeightmapData transform(bp::SubChunkPacket_<1001>::HeightmapData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::SubChunkPacketData, bp::SubChunkPacket_<2168>::SubChunkPacketData> {
    static bp::SubChunkPacket_<2168>::SubChunkPacketData transform(
        bp::SubChunkPacket_<1001>::SubChunkPacketData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::UncachedSubChunkPacketData,
                   bp::SubChunkPacket_<2168>::SubChunkPacketData> {
    static bp::SubChunkPacket_<2168>::SubChunkPacketData transform(
        bp::SubChunkPacket_<1001>::UncachedSubChunkPacketData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>, bp::SubChunkPacket_<2168>> {
    static bp::SubChunkPacket_<2168> transform(bp::SubChunkPacket_<1001> &&from);
};

template <>
struct Transformer<bp::SubChunkRequestPacket_<1001>, bp::SubChunkRequestPacket_<2168>> {
    static bp::SubChunkRequestPacket_<2168> transform(bp::SubChunkRequestPacket_<1001> &&from);
};

} // namespace endweave
