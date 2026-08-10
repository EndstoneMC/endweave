#include "endweave/protocols/v2168/chunk.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ew = endweave;

namespace endweave {
namespace {

constexpr std::uint32_t kSubChunkCountWhenClientRequesting = 0xffffffff;
constexpr std::uint32_t kPartialSubChunkCountWhenClientRequesting = 0xfffffffe;

} // namespace

void Transformer<bp::LevelChunkPacket_<2168>, bp::LevelChunkPacket_<1001>>::transform(
    Context<bp::LevelChunkPacket_<1001>> &ctx, bp::LevelChunkPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.pos = from.pos;
    to.dimension_id = from.dimension_id;
    // ENDWEAVE: 1001 marks a request by overwriting the count with a sentinel, so the optional becomes one.
    if (!from.client_request_sub_chunk_limit.has_value()) {
        to.sub_chunks_count = from.sub_chunks_count;
    }
    else if (from.client_request_sub_chunk_limit.value() < 0) {
        to.sub_chunks_count = kSubChunkCountWhenClientRequesting;
    }
    else {
        to.sub_chunks_count = kPartialSubChunkCountWhenClientRequesting;
        // ENDWEAVE: 1001 reserved sixteen bits for the limit, far above any sub-chunk index BDS asks for.
        to.client_request_sub_chunk_limit = static_cast<std::uint16_t>(from.client_request_sub_chunk_limit.value());
    }
    to.cache_enabled = from.cache_enabled;
    // ENDWEAVE: 1001 gates the blob list on the cache flag, so blobs sent with the cache off are dropped.
    to.cache_metadata = std::move(from.cache_metadata);
    to.serialized_chunk = std::move(from.serialized_chunk);
}

void Transformer<bp::SubChunkPacket_<2168>::SubChunkPosOffset, bp::SubChunkPacket_<1001>::SubChunkPosOffset>::transform(
    Context<bp::SubChunkPacket_<1001>::SubChunkPosOffset> &ctx, bp::SubChunkPacket_<2168>::SubChunkPosOffset &&from)
{
    auto &to = ctx.out();
    to.x = from.x;
    to.y = from.y;
    to.z = from.z;
}

void Transformer<bp::SubChunkPacket_<2168>::HeightmapData, bp::SubChunkPacket_<1001>::HeightmapData>::transform(
    Context<bp::SubChunkPacket_<1001>::HeightmapData> &ctx, bp::SubChunkPacket_<2168>::HeightmapData &&from)
{
    auto &to = ctx.out();
    to.height_map_type = static_cast<bp::SubChunkPacket_<1001>::HeightMapDataType>(from.height_map_type);
    // ENDWEAVE: TODO 1001 writes the samples only for HAS_DATA, so a HAS_DATA map with none goes out short.
    to.subchunk_height_map = std::move(from.subchunk_height_map).value_or(std::vector<std::int8_t>{});
    to.render_height_map_type = static_cast<bp::SubChunkPacket_<1001>::HeightMapDataType>(from.render_height_map_type);
    to.subchunk_render_height_map = std::move(from.subchunk_render_height_map).value_or(std::vector<std::int8_t>{});
}

void Transformer<bp::SubChunkPacket_<2168>::SubChunkPacketData, bp::SubChunkPacket_<1001>::SubChunkPacketData>::
    transform(Context<bp::SubChunkPacket_<1001>::SubChunkPacketData> &ctx,
              bp::SubChunkPacket_<2168>::SubChunkPacketData &&from)
{
    auto &to = ctx.out();
    to.sub_chunk_pos_offset = ew::transform(ctx, std::move(from.sub_chunk_pos_offset));
    to.result = static_cast<bp::SubChunkPacket_<1001>::SubChunkRequestResult>(from.result);
    // ENDWEAVE: 1001 demands a payload unless the result is all-air, so an absent one becomes zero-length.
    to.serialized_sub_chunk = std::move(from.serialized_sub_chunk).value_or(std::string{});
    to.height_map_data = ew::transform(ctx, std::move(from.height_map_data));
    // ENDWEAVE: 1001's cached entry always carries a blob id, and zero is what BDS uses when there is none.
    to.blob_id = from.blob_id.value_or(0);
}

void Transformer<bp::SubChunkPacket_<2168>, bp::SubChunkPacket_<1001>>::transform(
    Context<bp::SubChunkPacket_<1001>> &ctx, bp::SubChunkPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.cache_enabled = from.cache_enabled;
    to.dimension_type = from.dimension_type;
    to.center_pos_x = from.center_pos.x;
    to.center_pos_y = from.center_pos.y;
    to.center_pos_z = from.center_pos.z;
    // ENDWEAVE: the cache flag picks which of 1001's two entry lists is written. The uncached entry is
    // the cached one with its blob id dropped.
    auto entries = ew::transform_to<std::vector<bp::SubChunkPacket_<1001>::SubChunkPacketData>>(
        ctx, std::move(from.sub_chunk_data));
    if (from.cache_enabled) {
        to.sub_chunk_data = std::move(entries);
    }
    else {
        to.uncached_sub_chunk_data.reserve(entries.size());
        for (auto &entry : entries) {
            to.uncached_sub_chunk_data.push_back({.sub_chunk_pos_offset = entry.sub_chunk_pos_offset,
                                                  .result = entry.result,
                                                  .serialized_sub_chunk = std::move(entry.serialized_sub_chunk),
                                                  .height_map_data = std::move(entry.height_map_data)});
        }
    }
}

} // namespace endweave
