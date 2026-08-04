#include "endweave/protocols/v1001/chunk.h"

#include <cstdint>
#include <utility>

namespace ew = endweave;

namespace endweave {
namespace {

constexpr std::uint32_t kSubChunkCountWhenClientRequesting = 0xffffffff;
constexpr std::uint32_t kPartialSubChunkCountWhenClientRequesting = 0xfffffffe;
constexpr std::int32_t kUnlimitedSubChunkRequest = -1;

} // namespace

bp::LevelChunkPacket_<2168> Transformer<bp::LevelChunkPacket_<1001>>::upgrade(bp::LevelChunkPacket_<1001> &&from)
{
    bp::LevelChunkPacket_<2168> to;
    to.pos = from.pos;
    to.dimension_id = from.dimension_id;
    // ENDWEAVE: 1001's count doubles as the request marker, so a 2168 request sends zero and the optional carries it.
    if (from.sub_chunks_count == kSubChunkCountWhenClientRequesting) {
        to.sub_chunks_count = 0;
        to.client_request_sub_chunk_limit = kUnlimitedSubChunkRequest;
    }
    else if (from.sub_chunks_count == kPartialSubChunkCountWhenClientRequesting) {
        to.sub_chunks_count = 0;
        to.client_request_sub_chunk_limit = from.client_request_sub_chunk_limit;
    }
    else {
        to.sub_chunks_count = from.sub_chunks_count;
    }
    to.cache_enabled = from.cache_enabled;
    // ENDWEAVE: 2168 always writes the blob list, and a 1001 packet with the cache off decoded none.
    to.cache_metadata = std::move(from.cache_metadata);
    to.serialized_chunk = std::move(from.serialized_chunk);
    return to;
}

bp::SubChunkPacket_<2168>::SubChunkPosOffset Transformer<bp::SubChunkPacket_<1001>::SubChunkPosOffset>::upgrade(
    bp::SubChunkPacket_<1001>::SubChunkPosOffset &&from)
{
    bp::SubChunkPacket_<2168>::SubChunkPosOffset to;
    to.x = from.x;
    to.y = from.y;
    to.z = from.z;
    return to;
}

bp::SubChunkPacket_<2168>::HeightmapData Transformer<bp::SubChunkPacket_<1001>::HeightmapData>::upgrade(
    bp::SubChunkPacket_<1001>::HeightmapData &&from)
{
    using HeightMapDataType = bp::SubChunkPacket_<1001>::HeightMapDataType;
    bp::SubChunkPacket_<2168>::HeightmapData to;
    to.height_map_type = static_cast<bp::SubChunkPacket_<2168>::HeightMapDataType>(from.height_map_type);
    // ENDWEAVE: 1001 writes the 256 samples only for HAS_DATA, so that is what engages the 2168 optional.
    if (from.height_map_type == HeightMapDataType::HAS_DATA) {
        to.subchunk_height_map = std::move(from.subchunk_height_map);
    }
    to.render_height_map_type = static_cast<bp::SubChunkPacket_<2168>::HeightMapDataType>(from.render_height_map_type);
    if (from.render_height_map_type == HeightMapDataType::HAS_DATA) {
        to.subchunk_render_height_map = std::move(from.subchunk_render_height_map);
    }
    return to;
}

bp::SubChunkPacket_<2168>::SubChunkPacketData Transformer<bp::SubChunkPacket_<1001>::SubChunkPacketData>::upgrade(
    bp::SubChunkPacket_<1001>::SubChunkPacketData &&from)
{
    bp::SubChunkPacket_<2168>::SubChunkPacketData to;
    to.sub_chunk_pos_offset = ew::upgrade(from.sub_chunk_pos_offset);
    to.result = static_cast<bp::SubChunkPacket_<2168>::SubChunkRequestResult>(from.result);
    // ENDWEAVE: an all-air sub-chunk carries no payload at 1001, so the 2168 optional stays empty.
    if (from.result != bp::SubChunkPacket_<1001>::SubChunkRequestResult::SUCCESS_ALL_AIR) {
        to.serialized_sub_chunk = std::move(from.serialized_sub_chunk);
    }
    to.height_map_data = ew::upgrade(from.height_map_data);
    to.blob_id = from.blob_id;
    return to;
}

bp::SubChunkPacket_<2168>::SubChunkPacketData Transformer<bp::SubChunkPacket_<1001>::UncachedSubChunkPacketData>::
    upgrade(bp::SubChunkPacket_<1001>::UncachedSubChunkPacketData &&from)
{
    bp::SubChunkPacket_<2168>::SubChunkPacketData to;
    to.sub_chunk_pos_offset = ew::upgrade(from.sub_chunk_pos_offset);
    to.result = static_cast<bp::SubChunkPacket_<2168>::SubChunkRequestResult>(from.result);
    // ENDWEAVE: 1001's uncached entry writes its payload unconditionally, all-air included.
    to.serialized_sub_chunk = std::move(from.serialized_sub_chunk);
    to.height_map_data = ew::upgrade(from.height_map_data);
    // ENDWEAVE: blob_id belongs to the cached entry alone, so an uncached one reaches 2168 without it.
    return to;
}

bp::SubChunkPacket_<2168> Transformer<bp::SubChunkPacket_<1001>>::upgrade(bp::SubChunkPacket_<1001> &&from)
{
    bp::SubChunkPacket_<2168> to;
    to.cache_enabled = from.cache_enabled;
    to.dimension_type = from.dimension_type;
    to.center_pos = {.x = from.center_pos_x, .y = from.center_pos_y, .z = from.center_pos_z};
    // ENDWEAVE: 1001 fills only the list its cache flag names, so the flag picks the source rather than emptiness.
    if (from.cache_enabled) {
        to.sub_chunk_data = ew::upgrade(from.sub_chunk_data);
    }
    else {
        to.sub_chunk_data = ew::upgrade(from.uncached_sub_chunk_data);
    }
    return to;
}

bp::SubChunkRequestPacket_<2168> Transformer<bp::SubChunkRequestPacket_<1001>>::upgrade(
    bp::SubChunkRequestPacket_<1001> &&from)
{
    bp::SubChunkRequestPacket_<2168> to;
    to.dimension_type = from.dimension_type;
    to.sub_chunk_pos_offsets = ew::upgrade(from.sub_chunk_pos_offsets);
    to.center_pos = from.center_pos;
    return to;
}

} // namespace endweave
