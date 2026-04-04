#pragma once

/// Type tag structs for the template-based codec.
///
/// Each tag is a zero-size struct with a `value_type` alias. PacketReader::read<Tag>()
/// and PacketWriter::write<Tag>() are specialized per tag. No runtime objects needed.
///
/// See Also:
///     com.viaversion.viaversion.api.type.Type

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "endweave/codec/reader.h"
#include "endweave/codec/writer.h"

namespace endweave {

// -- Primitive type tags --

struct byte_t      { using value_type = std::uint8_t; };
struct bool_t      { using value_type = bool; };
struct short_le    { using value_type = std::int16_t; };
struct ushort_le   { using value_type = std::uint16_t; };
struct int_le      { using value_type = std::int32_t; };
struct int_be      { using value_type = std::int32_t; };
struct uint_le     { using value_type = std::uint32_t; };
struct int64_le    { using value_type = std::int64_t; };
struct float_le    { using value_type = float; };
struct double_le   { using value_type = double; };
struct var_int     { using value_type = std::int32_t; };
struct uvar_int    { using value_type = std::uint32_t; };
struct var_int64   { using value_type = std::int64_t; };
struct uvar_int64  { using value_type = std::uint64_t; };
struct string      { using value_type = std::string; };
struct remaining_bytes { using value_type = std::string_view; };

// Fixed-length bytes (UUID = 16 bytes)
struct uuid        { using value_type = std::array<std::uint8_t, 16>; };

// -- Compound position types --

using BlockPosValue = std::tuple<std::int32_t, std::int32_t, std::int32_t>;
using Vec3Value = std::tuple<float, float, float>;
using Vec2Value = std::tuple<float, float>;

/// v924 NetworkBlockPosition: varint X, uvarint Y, varint Z
struct network_block_pos { using value_type = BlockPosValue; };

/// v944 BlockPos: varint X, varint Y, varint Z
struct block_pos { using value_type = BlockPosValue; };

/// Vec3: three LE floats (X, Y, Z)
struct vec3 { using value_type = Vec3Value; };

/// Vec2: two LE floats (X, Y)
struct vec2 { using value_type = Vec2Value; };

// -- Optional types (bool prefix + value) --

struct optional_bool { using value_type = std::optional<bool>; };
struct optional_vec2 { using value_type = std::optional<Vec2Value>; };
struct optional_vec3 { using value_type = std::optional<Vec3Value>; };
struct optional_string { using value_type = std::optional<std::string>; };
struct optional_int_le { using value_type = std::optional<std::int32_t>; };
struct optional_float_le { using value_type = std::optional<float>; };
struct optional_byte { using value_type = std::optional<std::uint8_t>; };
struct optional_uvar_int64 { using value_type = std::optional<std::uint64_t>; };

// -- Reader specializations --

template <> inline auto PacketReader::read<byte_t>() -> std::expected<std::uint8_t, ReadError> {
    return read_byte();
}

template <> inline auto PacketReader::read<bool_t>() -> std::expected<bool, ReadError> {
    return read_bool();
}

template <> inline auto PacketReader::read<short_le>() -> std::expected<std::int16_t, ReadError> {
    return read_short_le();
}

template <> inline auto PacketReader::read<ushort_le>() -> std::expected<std::uint16_t, ReadError> {
    return read_ushort_le();
}

template <> inline auto PacketReader::read<int_le>() -> std::expected<std::int32_t, ReadError> {
    return read_int_le();
}

template <> inline auto PacketReader::read<int_be>() -> std::expected<std::int32_t, ReadError> {
    return read_int_be();
}

template <> inline auto PacketReader::read<uint_le>() -> std::expected<std::uint32_t, ReadError> {
    return read_uint_le();
}

template <> inline auto PacketReader::read<int64_le>() -> std::expected<std::int64_t, ReadError> {
    return read_int64_le();
}

template <> inline auto PacketReader::read<float_le>() -> std::expected<float, ReadError> {
    return read_float_le();
}

template <> inline auto PacketReader::read<double_le>() -> std::expected<double, ReadError> {
    return read_double_le();
}

template <> inline auto PacketReader::read<var_int>() -> std::expected<std::int32_t, ReadError> {
    return read_varint();
}

template <> inline auto PacketReader::read<uvar_int>() -> std::expected<std::uint32_t, ReadError> {
    return read_uvarint();
}

template <> inline auto PacketReader::read<var_int64>() -> std::expected<std::int64_t, ReadError> {
    return read_varint64();
}

template <> inline auto PacketReader::read<uvar_int64>() -> std::expected<std::uint64_t, ReadError> {
    return read_uvarint64();
}

template <> inline auto PacketReader::read<string>() -> std::expected<std::string, ReadError> {
    return read_string();
}

template <> inline auto PacketReader::read<remaining_bytes>() -> std::expected<std::string_view, ReadError> {
    return read_remaining();
}

template <> inline auto PacketReader::read<uuid>() -> std::expected<std::array<std::uint8_t, 16>, ReadError> {
    auto bytes = read_bytes(16);
    if (!bytes) return std::unexpected(bytes.error());
    std::array<std::uint8_t, 16> result;
    std::memcpy(result.data(), bytes->data(), 16);
    return result;
}

template <> inline auto PacketReader::read<network_block_pos>() -> std::expected<BlockPosValue, ReadError> {
    auto x = read_varint();
    if (!x) return std::unexpected(x.error());
    auto y_raw = read_uvarint();
    if (!y_raw) return std::unexpected(y_raw.error());
    // Unsigned to signed conversion for Y
    auto y = static_cast<std::int32_t>(*y_raw);
    auto z = read_varint();
    if (!z) return std::unexpected(z.error());
    return BlockPosValue{*x, y, *z};
}

template <> inline auto PacketReader::read<block_pos>() -> std::expected<BlockPosValue, ReadError> {
    auto x = read_varint();
    if (!x) return std::unexpected(x.error());
    auto y = read_varint();
    if (!y) return std::unexpected(y.error());
    auto z = read_varint();
    if (!z) return std::unexpected(z.error());
    return BlockPosValue{*x, *y, *z};
}

template <> inline auto PacketReader::read<vec3>() -> std::expected<Vec3Value, ReadError> {
    auto x = read_float_le();
    if (!x) return std::unexpected(x.error());
    auto y = read_float_le();
    if (!y) return std::unexpected(y.error());
    auto z = read_float_le();
    if (!z) return std::unexpected(z.error());
    return Vec3Value{*x, *y, *z};
}

template <> inline auto PacketReader::read<vec2>() -> std::expected<Vec2Value, ReadError> {
    auto x = read_float_le();
    if (!x) return std::unexpected(x.error());
    auto y = read_float_le();
    if (!y) return std::unexpected(y.error());
    return Vec2Value{*x, *y};
}

// -- Writer specializations --

template <> inline void PacketWriter::write<byte_t>(const std::uint8_t& val) {
    write_byte(val);
}

template <> inline void PacketWriter::write<bool_t>(const bool& val) {
    write_bool(val);
}

template <> inline void PacketWriter::write<short_le>(const std::int16_t& val) {
    write_short_le(val);
}

template <> inline void PacketWriter::write<ushort_le>(const std::uint16_t& val) {
    write_ushort_le(val);
}

template <> inline void PacketWriter::write<int_le>(const std::int32_t& val) {
    write_int_le(val);
}

template <> inline void PacketWriter::write<int_be>(const std::int32_t& val) {
    write_int_be(val);
}

template <> inline void PacketWriter::write<uint_le>(const std::uint32_t& val) {
    write_uint_le(val);
}

template <> inline void PacketWriter::write<int64_le>(const std::int64_t& val) {
    write_int64_le(val);
}

template <> inline void PacketWriter::write<float_le>(const float& val) {
    write_float_le(val);
}

template <> inline void PacketWriter::write<double_le>(const double& val) {
    write_double_le(val);
}

template <> inline void PacketWriter::write<var_int>(const std::int32_t& val) {
    write_varint(val);
}

template <> inline void PacketWriter::write<uvar_int>(const std::uint32_t& val) {
    write_uvarint(val);
}

template <> inline void PacketWriter::write<var_int64>(const std::int64_t& val) {
    write_varint64(val);
}

template <> inline void PacketWriter::write<uvar_int64>(const std::uint64_t& val) {
    write_uvarint64(val);
}

template <> inline void PacketWriter::write<string>(const std::string& val) {
    write_string(val);
}

template <> inline void PacketWriter::write<remaining_bytes>(const std::string_view& val) {
    write_bytes(val);
}

template <> inline void PacketWriter::write<uuid>(const std::array<std::uint8_t, 16>& val) {
    write_bytes({reinterpret_cast<const char*>(val.data()), 16});
}

template <> inline void PacketWriter::write<network_block_pos>(const BlockPosValue& val) {
    write_varint(std::get<0>(val));
    write_uvarint(static_cast<std::uint32_t>(std::get<1>(val)));
    write_varint(std::get<2>(val));
}

template <> inline void PacketWriter::write<block_pos>(const BlockPosValue& val) {
    write_varint(std::get<0>(val));
    write_varint(std::get<1>(val));
    write_varint(std::get<2>(val));
}

template <> inline void PacketWriter::write<vec3>(const Vec3Value& val) {
    write_float_le(std::get<0>(val));
    write_float_le(std::get<1>(val));
    write_float_le(std::get<2>(val));
}

template <> inline void PacketWriter::write<vec2>(const Vec2Value& val) {
    write_float_le(std::get<0>(val));
    write_float_le(std::get<1>(val));
}

// -- Optional reader/writer specializations --

template <> inline auto PacketReader::read<optional_bool>() -> std::expected<std::optional<bool>, ReadError> {
    auto has = read_bool();
    if (!has) return std::unexpected(has.error());
    if (!*has) return std::optional<bool>{};
    auto v = read_bool();
    if (!v) return std::unexpected(v.error());
    return std::optional<bool>{*v};
}

template <> inline auto PacketReader::read<optional_vec2>() -> std::expected<std::optional<Vec2Value>, ReadError> {
    auto has = read_bool();
    if (!has) return std::unexpected(has.error());
    if (!*has) return std::optional<Vec2Value>{};
    auto v = read<vec2>();
    if (!v) return std::unexpected(v.error());
    return std::optional<Vec2Value>{*v};
}

template <> inline auto PacketReader::read<optional_vec3>() -> std::expected<std::optional<Vec3Value>, ReadError> {
    auto has = read_bool();
    if (!has) return std::unexpected(has.error());
    if (!*has) return std::optional<Vec3Value>{};
    auto v = read<vec3>();
    if (!v) return std::unexpected(v.error());
    return std::optional<Vec3Value>{*v};
}

template <> inline void PacketWriter::write<optional_bool>(const std::optional<bool>& val) {
    if (val.has_value()) { write_bool(true); write_bool(*val); } else { write_bool(false); }
}

template <> inline void PacketWriter::write<optional_vec2>(const std::optional<Vec2Value>& val) {
    if (val.has_value()) { write_bool(true); write<vec2>(*val); } else { write_bool(false); }
}

template <> inline void PacketWriter::write<optional_vec3>(const std::optional<Vec3Value>& val) {
    if (val.has_value()) { write_bool(true); write<vec3>(*val); } else { write_bool(false); }
}

// -- Additional optional reader/writer specializations --

template <> inline auto PacketReader::read<optional_string>() -> std::expected<std::optional<std::string>, ReadError> {
    auto has = read_bool();
    if (!has) return std::unexpected(has.error());
    if (!*has) return std::optional<std::string>{};
    auto v = read_string();
    if (!v) return std::unexpected(v.error());
    return std::optional<std::string>{std::move(*v)};
}

template <> inline void PacketWriter::write<optional_string>(const std::optional<std::string>& val) {
    if (val.has_value()) { write_bool(true); write_string(*val); } else { write_bool(false); }
}

template <> inline auto PacketReader::read<optional_int_le>() -> std::expected<std::optional<std::int32_t>, ReadError> {
    auto has = read_bool();
    if (!has) return std::unexpected(has.error());
    if (!*has) return std::optional<std::int32_t>{};
    auto v = read_int_le();
    if (!v) return std::unexpected(v.error());
    return std::optional<std::int32_t>{*v};
}

template <> inline void PacketWriter::write<optional_int_le>(const std::optional<std::int32_t>& val) {
    if (val.has_value()) { write_bool(true); write_int_le(*val); } else { write_bool(false); }
}

template <> inline auto PacketReader::read<optional_float_le>() -> std::expected<std::optional<float>, ReadError> {
    auto has = read_bool();
    if (!has) return std::unexpected(has.error());
    if (!*has) return std::optional<float>{};
    auto v = read_float_le();
    if (!v) return std::unexpected(v.error());
    return std::optional<float>{*v};
}

template <> inline void PacketWriter::write<optional_float_le>(const std::optional<float>& val) {
    if (val.has_value()) { write_bool(true); write_float_le(*val); } else { write_bool(false); }
}

template <> inline auto PacketReader::read<optional_byte>() -> std::expected<std::optional<std::uint8_t>, ReadError> {
    auto has = read_bool();
    if (!has) return std::unexpected(has.error());
    if (!*has) return std::optional<std::uint8_t>{};
    auto v = read_byte();
    if (!v) return std::unexpected(v.error());
    return std::optional<std::uint8_t>{*v};
}

template <> inline void PacketWriter::write<optional_byte>(const std::optional<std::uint8_t>& val) {
    if (val.has_value()) { write_bool(true); write_byte(*val); } else { write_bool(false); }
}

template <> inline auto PacketReader::read<optional_uvar_int64>() -> std::expected<std::optional<std::uint64_t>, ReadError> {
    auto has = read_bool();
    if (!has) return std::unexpected(has.error());
    if (!*has) return std::optional<std::uint64_t>{};
    auto v = read_uvarint64();
    if (!v) return std::unexpected(v.error());
    return std::optional<std::uint64_t>{*v};
}

template <> inline void PacketWriter::write<optional_uvar_int64>(const std::optional<std::uint64_t>& val) {
    if (val.has_value()) { write_bool(true); write_uvarint64(*val); } else { write_bool(false); }
}

} // namespace endweave
