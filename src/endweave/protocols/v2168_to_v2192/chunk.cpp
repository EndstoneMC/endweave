#include "chunk.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace ew = endweave;

namespace endweave {
namespace {

/** 2168 writes sixteen fixed-size rows; 2192 length-prefixes each row. */
std::optional<std::array<std::vector<std::int8_t>, 16>> rowsOf(
    std::optional<std::array<std::array<std::int8_t, 16>, 16>> &&fixed)
{
    if (!fixed.has_value()) {
        return std::nullopt;
    }
    std::array<std::vector<std::int8_t>, 16> rows;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        rows[i].assign((*fixed)[i].begin(), (*fixed)[i].end());
    }
    return rows;
}

} // namespace

void Transformer<bp::SubChunkPacket_<2168>::HeightmapData, bp::SubChunkPacket_<2192>::HeightmapData>::transform(
    Context<bp::SubChunkPacket_<2192>::HeightmapData> &ctx, bp::SubChunkPacket_<2168>::HeightmapData &&from)
{
    auto &to = ctx.out();
    to.height_map_type = from.height_map_type;
    to.subchunk_height_map = rowsOf(std::move(from.subchunk_height_map));
    to.render_height_map_type = from.render_height_map_type;
    to.subchunk_render_height_map = rowsOf(std::move(from.subchunk_render_height_map));
}

} // namespace endweave
