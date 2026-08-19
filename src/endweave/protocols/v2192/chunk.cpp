#include "endweave/protocols/v2192/chunk.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace ew = endweave;

namespace endweave {
namespace {

/** 2168 wants the height map as one flat run, so the rows are laid end to end and the lengths 2192
 * carries in front of each are dropped. */
std::optional<std::vector<std::int8_t>> flattened(std::optional<std::vector<std::vector<std::int8_t>>> &&rows)
{
    if (!rows.has_value()) {
        return std::nullopt;
    }
    std::vector<std::int8_t> flat;
    std::size_t total = 0;
    for (const auto &row : *rows) {
        total += row.size();
    }
    flat.reserve(total);
    for (auto &row : *rows) {
        flat.insert(flat.end(), row.begin(), row.end());
    }
    return flat;
}

} // namespace

void Transformer<bp::SubChunkPacket_<2192>::HeightmapData, bp::SubChunkPacket_<2168>::HeightmapData>::transform(
    Context<bp::SubChunkPacket_<2168>::HeightmapData> &ctx, bp::SubChunkPacket_<2192>::HeightmapData &&from)
{
    auto &to = ctx.out();
    to.height_map_type = from.height_map_type;
    to.subchunk_height_map = flattened(std::move(from.subchunk_height_map));
    to.render_height_map_type = from.render_height_map_type;
    to.subchunk_render_height_map = flattened(std::move(from.subchunk_render_height_map));
}

} // namespace endweave
