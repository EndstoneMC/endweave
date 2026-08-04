#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/chunk.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::LevelChunkPacket_<1001>> {
    static bp::LevelChunkPacket_<2168> upgrade(bp::LevelChunkPacket_<1001> &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::SubChunkPosOffset> {
    static bp::SubChunkPacket_<2168>::SubChunkPosOffset upgrade(bp::SubChunkPacket_<1001>::SubChunkPosOffset &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::HeightmapData> {
    static bp::SubChunkPacket_<2168>::HeightmapData upgrade(bp::SubChunkPacket_<1001>::HeightmapData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::SubChunkPacketData> {
    static bp::SubChunkPacket_<2168>::SubChunkPacketData upgrade(bp::SubChunkPacket_<1001>::SubChunkPacketData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>::UncachedSubChunkPacketData> {
    static bp::SubChunkPacket_<2168>::SubChunkPacketData upgrade(
        bp::SubChunkPacket_<1001>::UncachedSubChunkPacketData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<1001>> {
    static bp::SubChunkPacket_<2168> upgrade(bp::SubChunkPacket_<1001> &&from);
};

template <>
struct Transformer<bp::SubChunkRequestPacket_<1001>> {
    static bp::SubChunkRequestPacket_<2168> upgrade(bp::SubChunkRequestPacket_<1001> &&from);
};

} // namespace endweave
