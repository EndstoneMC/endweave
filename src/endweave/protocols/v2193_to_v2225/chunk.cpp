#include "chunk.h"

#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::LevelChunkPacket_<2193>, bp::LevelChunkPacket_<2225>>::transform(
    Context<bp::LevelChunkPacket_<2225>> &ctx, bp::LevelChunkPacket_<2193> &&from)
{
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    // ENDWEAVE: 2193 has no client biome updates, so every chunk it sends is a whole one.
    to.is_client_biome_update = false;
}

} // namespace endweave
