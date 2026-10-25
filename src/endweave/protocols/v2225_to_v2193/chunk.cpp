#include "chunk.h"

#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::LevelChunkPacket_<2225>, bp::LevelChunkPacket_<2193>>::transform(
    Context<bp::LevelChunkPacket_<2193>> &ctx, bp::LevelChunkPacket_<2225> &&from)
{
    // ENDWEAVE: 2193 reads every LevelChunkPacket as the whole chunk, so a biome update is dropped and
    // the client keeps the biomes it has until the chunk is sent again.
    if (from.is_client_biome_update) {
        ctx.cancel();
        return;
    }
    ew::transform_members(ctx, std::move(from), ctx.out());
}

} // namespace endweave
