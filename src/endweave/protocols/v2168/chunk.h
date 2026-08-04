#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/chunk.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::LevelChunkPacket_<2168>> {
    static bp::LevelChunkPacket_<1001> downgrade(bp::LevelChunkPacket_<2168> &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>::SubChunkPosOffset> {
    static bp::SubChunkPacket_<1001>::SubChunkPosOffset downgrade(bp::SubChunkPacket_<2168>::SubChunkPosOffset &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>::HeightmapData> {
    static bp::SubChunkPacket_<1001>::HeightmapData downgrade(bp::SubChunkPacket_<2168>::HeightmapData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>::SubChunkPacketData> {
    static bp::SubChunkPacket_<1001>::SubChunkPacketData downgrade(
        bp::SubChunkPacket_<2168>::SubChunkPacketData &&from);
};

template <>
struct Transformer<bp::SubChunkPacket_<2168>> {
    static bp::SubChunkPacket_<1001> downgrade(bp::SubChunkPacket_<2168> &&from);
};

template <>
struct Transformer<bp::SubChunkRequestPacket_<2168>> {
    static bp::SubChunkRequestPacket_<1001> downgrade(bp::SubChunkRequestPacket_<2168> &&from);
};

} // namespace endweave
