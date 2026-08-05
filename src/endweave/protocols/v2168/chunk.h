#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/chunk.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::LevelChunkPacket_<2168>, bp::LevelChunkPacket_<1001>> {
    static bp::LevelChunkPacket_<1001> transform(bp::LevelChunkPacket_<2168> &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>::SubChunkPosOffset, bp::SubChunkPacket_<1001>::SubChunkPosOffset> {
    static bp::SubChunkPacket_<1001>::SubChunkPosOffset transform(bp::SubChunkPacket_<2168>::SubChunkPosOffset &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>::HeightmapData, bp::SubChunkPacket_<1001>::HeightmapData> {
    static bp::SubChunkPacket_<1001>::HeightmapData transform(bp::SubChunkPacket_<2168>::HeightmapData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>::SubChunkPacketData, bp::SubChunkPacket_<1001>::SubChunkPacketData> {
    static bp::SubChunkPacket_<1001>::SubChunkPacketData transform(
        bp::SubChunkPacket_<2168>::SubChunkPacketData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>, bp::SubChunkPacket_<1001>> {
    static bp::SubChunkPacket_<1001> transform(bp::SubChunkPacket_<2168> &&from);
};

template <>
struct Transformer<bp::SubChunkRequestPacket_<2168>, bp::SubChunkRequestPacket_<1001>> {
    static bp::SubChunkRequestPacket_<1001> transform(bp::SubChunkRequestPacket_<2168> &&from);
};

} // namespace endweave
