#include "endweave/protocols/v1001/skin.h"

#include <charconv>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace ew = endweave;

namespace endweave {
namespace {

std::uint32_t parseHex(std::string_view text)
{
    std::uint32_t out = 0;
    std::from_chars(text.data(), text.data() + text.size(), out, 16);
    return out;
}

// The two forms hold the same ARGB int: cereal binds mce::Color through setARGB / toARGB,
// and the pre-cereal write is toHexString, which is '#' then that int in hex, unpadded.
// fromHexString drops the first character and reads the top byte as alpha, so a six-digit
// colour is transparent rather than opaque.
bp::Color colorFromHex(std::string_view text)
{
    if (!text.empty() && text.front() == '#') {
        text.remove_prefix(1);
    }
    return static_cast<bp::Color>(parseHex(text));
}

std::string colorToHex(bp::Color color)
{
    return std::format("#{:x}", static_cast<std::uint32_t>(color));
}

bp::UUID uuidFromString(std::string_view text)
{
    std::string digits;
    digits.reserve(32);
    for (const char c : text) {
        if (c != '-') {
            digits.push_back(c);
        }
    }
    if (digits.size() != 32) {
        return {};
    }
    bp::UUID out;
    std::from_chars(digits.data(), digits.data() + 16, out.most_significant_bits, 16);
    std::from_chars(digits.data() + 16, digits.data() + 32, out.least_significant_bits, 16);
    return out;
}

std::string uuidToString(const bp::UUID &uuid)
{
    return std::format(
        "{:08x}-{:04x}-{:04x}-{:04x}-{:012x}", static_cast<std::uint32_t>(uuid.most_significant_bits >> 32),
        static_cast<std::uint16_t>(uuid.most_significant_bits >> 16),
        static_cast<std::uint16_t>(uuid.most_significant_bits),
        static_cast<std::uint16_t>(uuid.least_significant_bits >> 48), uuid.least_significant_bits & 0xFFFFFFFFFFFFull);
}

bp::SkinImage imageOf(std::uint32_t width, std::uint32_t height, std::string &&bytes)
{
    bp::SkinImage to;
    to.width = width;
    to.height = height;
    to.image_bytes = std::move(bytes);
    return to;
}

} // namespace

bp::AnimatedImageData Transformer<bp::legacy::AnimatedImageData>::toCereal(bp::legacy::AnimatedImageData &&from)
{
    bp::AnimatedImageData to;
    to.image = imageOf(from.image_width, from.image_height, std::move(from.image_bytes));
    to.type = from.type;
    to.frames = from.frames;
    to.animation_expression = from.animation_expression;
    return to;
}

bp::legacy::AnimatedImageData Transformer<bp::AnimatedImageData>::toLegacy(bp::AnimatedImageData &&from)
{
    bp::legacy::AnimatedImageData to;
    to.image_width = from.image.width;
    to.image_height = from.image.height;
    to.image_bytes = std::move(from.image.image_bytes);
    to.type = from.type;
    to.frames = from.frames;
    to.animation_expression = from.animation_expression;
    return to;
}

bp::SerializedPersonaPieceHandle Transformer<bp::legacy::SerializedPersonaPieceHandle>::toCereal(
    bp::legacy::SerializedPersonaPieceHandle &&from)
{
    bp::SerializedPersonaPieceHandle to;
    to.piece_id = std::move(from.piece_id);
    to.piece_type = from.piece_type;
    to.pack_id = uuidFromString(from.pack_id);
    to.is_default_piece = from.is_default_piece;
    to.product_id = std::move(from.product_id);
    return to;
}

bp::legacy::SerializedPersonaPieceHandle Transformer<bp::SerializedPersonaPieceHandle>::toLegacy(
    bp::SerializedPersonaPieceHandle &&from)
{
    bp::legacy::SerializedPersonaPieceHandle to;
    to.piece_id = std::move(from.piece_id);
    to.piece_type = from.piece_type;
    to.pack_id = uuidToString(from.pack_id);
    to.is_default_piece = from.is_default_piece;
    to.product_id = std::move(from.product_id);
    return to;
}

bp::TintMapColor Transformer<bp::legacy::TintMapColor>::toCereal(bp::legacy::TintMapColor &&from)
{
    bp::TintMapColor to;
    for (const auto &hex : from.colors) {
        to.colors.push_back(colorFromHex(hex));
    }
    // ENDWEAVE: BDS holds four colours per piece and the cerealised form writes exactly four behind
    // no count, so a shorter list would put a skin on the wire the client cannot read.
    to.colors.resize(4);
    return to;
}

bp::legacy::TintMapColor Transformer<bp::TintMapColor>::toLegacy(bp::TintMapColor &&from)
{
    bp::legacy::TintMapColor to;
    // ENDWEAVE: the piece type is the map key at the cerealised form, so the skin fills it in.
    to.piece_type = {};
    for (const bp::Color color : from.colors) {
        to.colors.push_back(colorToHex(color));
    }
    return to;
}

bp::SerializedSkinRef_<1001> Transformer<bp::legacy::SerializedSkinRef>::toCereal(bp::legacy::SerializedSkinRef &&from)
{
    bp::SerializedSkinRef_<1001> to;
    to.id = std::move(from.id);
    to.play_fab_id = std::move(from.play_fab_id);
    to.resource_patch = std::move(from.resource_patch);
    to.image_data = imageOf(from.image_width, from.image_height, std::move(from.image_bytes));
    to.animated_image_data = ew::toCereal(from.animated_image_data);
    to.cape_image_data = imageOf(from.cape_image_width, from.cape_image_height, std::move(from.cape_image_bytes));
    to.geometry_data = std::move(from.geometry_data);
    to.geometry_data_min_engine_version = std::move(from.geometry_data_min_engine_version);
    to.animation_data = std::move(from.animation_data);
    to.cape_id = std::move(from.cape_id);
    to.full_id = std::move(from.full_id);
    to.arm_size = from.arm_size;
    to.skin_color = colorFromHex(from.skin_color);
    to.persona_pieces = ew::toCereal(from.persona_pieces);
    // The pre-cereal write walks the map as a list, so each entry carries its own key back.
    for (auto &tint : from.piece_tint_colors) {
        to.piece_tint_colors.emplace(tint.piece_type, ew::toCereal(tint));
    }
    to.is_premium = from.is_premium;
    to.is_persona = from.is_persona;
    to.is_persona_cape_on_classic_skin = from.is_persona_cape_on_classic_skin;
    to.is_primary_user = from.is_primary_user;
    to.overrides_player_appearance = from.overrides_player_appearance;
    // ENDWEAVE: the pre-cereal skin has no flag of its own, so PlayerListPacket fills it in from
    // the trailing run it writes one bool per entry into.
    to.trusted_skin_flag = bp::TrustedSkinFlag::UNSET;
    return to;
}

bp::legacy::SerializedSkinRef Transformer<bp::SerializedSkinRef_<1001>>::toLegacy(bp::SerializedSkinRef_<1001> &&from)
{
    bp::legacy::SerializedSkinRef to;
    to.id = std::move(from.id);
    to.play_fab_id = std::move(from.play_fab_id);
    to.resource_patch = std::move(from.resource_patch);
    to.image_width = from.image_data.width;
    to.image_height = from.image_data.height;
    to.image_bytes = std::move(from.image_data.image_bytes);
    to.animated_image_data = ew::toLegacy(from.animated_image_data);
    to.cape_image_width = from.cape_image_data.width;
    to.cape_image_height = from.cape_image_data.height;
    to.cape_image_bytes = std::move(from.cape_image_data.image_bytes);
    to.geometry_data = std::move(from.geometry_data);
    to.geometry_data_min_engine_version = std::move(from.geometry_data_min_engine_version);
    to.animation_data = std::move(from.animation_data);
    to.cape_id = std::move(from.cape_id);
    to.full_id = std::move(from.full_id);
    to.arm_size = from.arm_size;
    to.skin_color = colorToHex(from.skin_color);
    to.persona_pieces = ew::toLegacy(from.persona_pieces);
    for (auto &[piece_type, colors] : from.piece_tint_colors) {
        auto tint = ew::toLegacy(colors);
        tint.piece_type = piece_type;
        to.piece_tint_colors.push_back(std::move(tint));
    }
    to.is_premium = from.is_premium;
    to.is_persona = from.is_persona;
    to.is_persona_cape_on_classic_skin = from.is_persona_cape_on_classic_skin;
    to.is_primary_user = from.is_primary_user;
    to.overrides_player_appearance = from.overrides_player_appearance;
    // ENDWEAVE: trusted_skin_flag stops here; PlayerListPacket writes it as a trailing bool of its
    // own, and PlayerSkinPacket is cerealised at 1001 and never reaches this form.
    return to;
}

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
