#include "endweave/protocols/v1001/skin.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

bp::SerializedSkinRef_<2168> Transformer<bp::SerializedSkinRef_<1001>>::upgrade(bp::SerializedSkinRef_<1001> &&from)
{
    bp::SerializedSkinRef_<2168> to;
    to.id = std::move(from.id);
    to.play_fab_id = std::move(from.play_fab_id);
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
    to.persona_pieces = std::move(from.persona_pieces);
    to.piece_tint_colors = std::move(from.piece_tint_colors);
    to.is_premium = from.is_premium;
    to.is_persona = from.is_persona;
    to.is_persona_cape_on_classic_skin = from.is_persona_cape_on_classic_skin;
    to.is_primary_user = from.is_primary_user;
    to.overrides_player_appearance = from.overrides_player_appearance;
    to.trusted_skin_flag = from.trusted_skin_flag;
    // ENDWEAVE: TODO 1001 carries no profile hash. Empty is what a client with no persona
    // profile sends, so the server reads the skin as unhashed rather than as a mismatch.
    to.profile_hash = {};
    return to;
}

bp::PlayerSkinPacket_<2168> Transformer<bp::PlayerSkinPacket_<1001>>::upgrade(bp::PlayerSkinPacket_<1001> &&from)
{
    bp::PlayerSkinPacket_<2168> to;
    to.uuid = from.uuid;
    to.skin = ew::upgrade(from.skin);
    to.localized_new_skin_name = std::move(from.localized_new_skin_name);
    to.localized_old_skin_name = std::move(from.localized_old_skin_name);
    return to;
}

} // namespace endweave
