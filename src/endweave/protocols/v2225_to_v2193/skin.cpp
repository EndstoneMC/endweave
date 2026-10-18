#include "skin.h"

#include <bedrock/protocol/enum.hpp>
#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::SerializedSkinRef_<2225>, bp::SerializedSkinRef_<2193>>::transform(
    Context<bp::SerializedSkinRef_<2193>> &ctx, bp::SerializedSkinRef_<2225> &&from)
{
    using Piece = bp::PieceType_<2193>;
    auto &to = ctx.out();
    to.id = std::move(from.id);
    // ENDWEAVE: 2225 moved the PlayFab id onto the player list entry, which writes it back in. A skin
    // sent on its own has none to carry, so it is left empty.
    to.play_fab_id = {};
    to.resource_patch = std::move(from.resource_patch);
    to.image_data = std::move(from.image_data);
    to.animated_image_data = std::move(from.animated_image_data);
    to.cape_image_data = std::move(from.cape_image_data);
    to.geometry_data = std::move(from.geometry_data);
    to.geometry_data_min_engine_version = std::move(from.geometry_data_min_engine_version);
    to.animation_data = std::move(from.animation_data);
    to.cape_id = std::move(from.cape_id);
    to.full_id = std::move(from.full_id);
    to.arm_size = from.arm_size;
    to.skin_color = from.skin_color;
    to.persona_pieces = ew::transform(ctx, std::move(from.persona_pieces));
    for (auto &[piece, tint] : from.piece_tint_colors) {
        // ENDWEAVE: 2193 reads the key by name, so a Coco tint is dropped rather than sent as one it
        // cannot decode.
        if (const auto type = bp::enum_cast<Piece>(bp::enum_name(piece))) {
            to.piece_tint_colors.emplace(*type, std::move(tint));
        }
    }
    to.is_premium = from.is_premium;
    to.is_persona = from.is_persona;
    to.is_persona_cape_on_classic_skin = from.is_persona_cape_on_classic_skin;
    to.is_primary_user = from.is_primary_user;
    to.overrides_player_appearance = from.overrides_player_appearance;
    to.trusted_skin_flag = from.trusted_skin_flag;
    to.profile_hash = std::move(from.profile_hash);
}

} // namespace endweave
