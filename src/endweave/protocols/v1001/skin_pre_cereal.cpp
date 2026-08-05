#include "endweave/protocols/v1001/skin_pre_cereal.h"

#include <charconv>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace endweave {
namespace {

std::uint32_t parseHex(std::string_view text)
{
    std::uint32_t out = 0;
    std::from_chars(text.data(), text.data() + text.size(), out, 16);
    return out;
}

// ENDWEAVE: TODO mce::Color::toHexString has not been read out of BDS. gophertunnel
// documents a skin colour as #RRGGBB and a tint colour as #AARRGGBB, and shows "#0" for
// an unused tint slot, so the width varies and the alpha is only sometimes there. Six
// digits or fewer are read as opaque; a wrong guess costs a tint, not a connection.
bp::Color colorFromHex(std::string_view text)
{
    if (!text.empty() && text.front() == '#') {
        text.remove_prefix(1);
    }
    const std::uint32_t packed = parseHex(text);
    return static_cast<bp::Color>(text.size() > 6 ? packed : packed | 0xFF000000u);
}

std::string colorToHex(bp::Color color)
{
    return std::format("#{:08x}", static_cast<std::uint32_t>(color));
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
    return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}", static_cast<std::uint32_t>(uuid.most_significant_bits >> 32),
                       static_cast<std::uint16_t>(uuid.most_significant_bits >> 16),
                       static_cast<std::uint16_t>(uuid.most_significant_bits),
                       static_cast<std::uint16_t>(uuid.least_significant_bits >> 48),
                       uuid.least_significant_bits & 0xFFFFFFFFFFFFull);
}

bp::SkinImage imageOf(std::uint32_t width, std::uint32_t height, std::string &&bytes)
{
    bp::SkinImage out;
    out.width = width;
    out.height = height;
    out.image_bytes = std::move(bytes);
    return out;
}

bp::AnimatedImageData cerealizeAnimation(bp::legacy::AnimatedImageData &&from)
{
    bp::AnimatedImageData to;
    to.image = imageOf(from.image_width, from.image_height, std::move(from.image_bytes));
    to.type = from.type;
    to.frames = from.frames;
    to.animation_expression = from.animation_expression;
    return to;
}

bp::legacy::AnimatedImageData decerealizeAnimation(bp::AnimatedImageData &&from)
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

bp::SerializedPersonaPieceHandle cerealizePiece(bp::legacy::SerializedPersonaPieceHandle &&from)
{
    bp::SerializedPersonaPieceHandle to;
    to.piece_id = std::move(from.piece_id);
    to.piece_type = from.piece_type;
    to.pack_id = uuidFromString(from.pack_id);
    to.is_default_piece = from.is_default_piece;
    to.product_id = std::move(from.product_id);
    return to;
}

bp::legacy::SerializedPersonaPieceHandle decerealizePiece(bp::SerializedPersonaPieceHandle &&from)
{
    bp::legacy::SerializedPersonaPieceHandle to;
    to.piece_id = std::move(from.piece_id);
    to.piece_type = from.piece_type;
    to.pack_id = uuidToString(from.pack_id);
    to.is_default_piece = from.is_default_piece;
    to.product_id = std::move(from.product_id);
    return to;
}

} // namespace

bool isTrusted(const bp::SerializedSkinRef_<1001> &skin)
{
    return skin.trusted_skin_flag == bp::TrustedSkinFlag::TRUE;
}

bp::SerializedSkinRef_<1001> cerealize(bp::legacy::SerializedSkinRef &&from, bool trusted)
{
    bp::SerializedSkinRef_<1001> to;
    to.id = std::move(from.id);
    to.play_fab_id = std::move(from.play_fab_id);
    to.resource_patch = std::move(from.resource_patch);
    to.image_data = imageOf(from.image_width, from.image_height, std::move(from.image_bytes));
    for (auto &animation : from.animated_image_data) {
        to.animated_image_data.push_back(cerealizeAnimation(std::move(animation)));
    }
    to.cape_image_data = imageOf(from.cape_image_width, from.cape_image_height, std::move(from.cape_image_bytes));
    to.geometry_data = std::move(from.geometry_data);
    to.geometry_data_min_engine_version = std::move(from.geometry_data_min_engine_version);
    to.animation_data = std::move(from.animation_data);
    to.cape_id = std::move(from.cape_id);
    to.full_id = std::move(from.full_id);
    to.arm_size = from.arm_size;
    to.skin_color = colorFromHex(from.skin_color);
    for (auto &piece : from.persona_pieces) {
        to.persona_pieces.push_back(cerealizePiece(std::move(piece)));
    }
    // The pre-cereal write walks the map as a list, so each entry carries the key back.
    for (auto &tint : from.piece_tint_colors) {
        bp::TintMapColor colors;
        for (const auto &hex : tint.colors) {
            colors.colors.push_back(colorFromHex(hex));
        }
        to.piece_tint_colors.emplace(tint.piece_type, std::move(colors));
    }
    to.is_premium = from.is_premium;
    to.is_persona = from.is_persona;
    to.is_persona_cape_on_classic_skin = from.is_persona_cape_on_classic_skin;
    to.is_primary_user = from.is_primary_user;
    to.overrides_player_appearance = from.overrides_player_appearance;
    // The pre-cereal skin has no flag of its own: the packet carries one bool per entry.
    to.trusted_skin_flag = trusted ? bp::TrustedSkinFlag::TRUE : bp::TrustedSkinFlag::FALSE;
    return to;
}

bp::legacy::SerializedSkinRef decerealize(bp::SerializedSkinRef_<1001> &&from)
{
    bp::legacy::SerializedSkinRef to;
    to.id = std::move(from.id);
    to.play_fab_id = std::move(from.play_fab_id);
    to.resource_patch = std::move(from.resource_patch);
    to.image_width = from.image_data.width;
    to.image_height = from.image_data.height;
    to.image_bytes = std::move(from.image_data.image_bytes);
    for (auto &animation : from.animated_image_data) {
        to.animated_image_data.push_back(decerealizeAnimation(std::move(animation)));
    }
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
    for (auto &piece : from.persona_pieces) {
        to.persona_pieces.push_back(decerealizePiece(std::move(piece)));
    }
    for (auto &[piece_type, colors] : from.piece_tint_colors) {
        bp::legacy::TintMapColor tint;
        tint.piece_type = piece_type;
        for (const bp::Color color : colors.colors) {
            tint.colors.push_back(colorToHex(color));
        }
        to.piece_tint_colors.push_back(std::move(tint));
    }
    to.is_premium = from.is_premium;
    to.is_persona = from.is_persona;
    to.is_persona_cape_on_classic_skin = from.is_persona_cape_on_classic_skin;
    to.is_primary_user = from.is_primary_user;
    to.overrides_player_appearance = from.overrides_player_appearance;
    return to;
}

} // namespace endweave
