#include "skin.h"

#include <bedrock/protocol/enum.hpp>
#include <map>
#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::SerializedSkinRef_<2225>, bp::SerializedSkinRef_<2193>>::transform(
    Context<bp::SerializedSkinRef_<2193>> &ctx, bp::SerializedSkinRef_<2225> &&from)
{
    // ENDWEAVE: 2193 reads the key by name, so a tint of a piece it cannot name is dropped rather than
    // sent as one it cannot decode.
    std::erase_if(from.piece_tint_colors, [](const auto &tint) {
        return !bp::enum_name_<2193>(tint.first);
    });
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    // ENDWEAVE: 2225 moved the PlayFab id onto the player list entry, which writes it back in. A skin
    // sent on its own has none to carry, so it is left empty.
    to.play_fab_id = {};
}

} // namespace endweave
