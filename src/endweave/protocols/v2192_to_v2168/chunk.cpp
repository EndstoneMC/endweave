#include "endweave/protocols/v2192_to_v2168/chunk.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace ew = endweave;

namespace endweave {
namespace {

/** 2168 writes sixteen values a row behind no length, so a row that arrived shorter leaves the rest
 * of its sixteen at zero rather than putting a row the client cannot read on the wire. */
std::optional<std::array<std::array<std::int8_t, 16>, 16>> fixedRows(
    std::optional<std::array<std::vector<std::int8_t>, 16>> &&rows)
{
    if (!rows.has_value()) {
        return std::nullopt;
    }
    std::array<std::array<std::int8_t, 16>, 16> fixed{};
    for (std::size_t i = 0; i < fixed.size(); ++i) {
        const auto &row = (*rows)[i];
        const std::size_t carried = row.size() < fixed[i].size() ? row.size() : fixed[i].size();
        for (std::size_t j = 0; j < carried; ++j) {
            fixed[i][j] = row[j];
        }
    }
    return fixed;
}

} // namespace

void Transformer<bp::SubChunkPacket_<2192>::HeightmapData, bp::SubChunkPacket_<2168>::HeightmapData>::transform(
    Context<bp::SubChunkPacket_<2168>::HeightmapData> &ctx, bp::SubChunkPacket_<2192>::HeightmapData &&from)
{
    auto &to = ctx.out();
    to.height_map_type = from.height_map_type;
    to.subchunk_height_map = fixedRows(std::move(from.subchunk_height_map));
    to.render_height_map_type = from.render_height_map_type;
    to.subchunk_render_height_map = fixedRows(std::move(from.subchunk_render_height_map));
}

} // namespace endweave
