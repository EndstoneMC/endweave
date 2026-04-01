#pragma once

/// Binary packet reader for Bedrock protocol deserialization.
///
/// Wraps a std::string_view for zero-copy reads over Endstone's payload buffer.
/// All read methods return std::expected to avoid exception overhead on the hot path.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <string>
#include <string_view>

namespace endweave {

/// Error returned when a read operation fails.
enum class ReadError {
    out_of_bounds,
    varint_too_long,
    string_too_long,
};

class PacketReader {
public:
    explicit PacketReader(std::string_view data) : data_(data) {}

    [[nodiscard]] std::size_t position() const { return pos_; }
    void set_position(std::size_t pos) { pos_ = pos; }
    [[nodiscard]] bool has_remaining() const { return pos_ < data_.size(); }
    [[nodiscard]] std::size_t remaining() const { return data_.size() - pos_; }

    std::expected<void, ReadError> skip(std::size_t n) {
        if (n > remaining()) return std::unexpected(ReadError::out_of_bounds);
        pos_ += n;
        return {};
    }

    std::expected<std::uint8_t, ReadError> read_byte() {
        if (pos_ >= data_.size()) return std::unexpected(ReadError::out_of_bounds);
        return static_cast<std::uint8_t>(data_[pos_++]);
    }

    std::expected<std::string_view, ReadError> read_bytes(std::size_t n) {
        if (n > remaining()) return std::unexpected(ReadError::out_of_bounds);
        auto result = data_.substr(pos_, n);
        pos_ += n;
        return result;
    }

    std::string_view read_remaining() {
        auto result = data_.substr(pos_);
        pos_ = data_.size();
        return result;
    }

    std::expected<bool, ReadError> read_bool() {
        auto b = read_byte();
        if (!b) return std::unexpected(b.error());
        return *b != 0;
    }

    std::expected<std::int16_t, ReadError> read_short_le() {
        return read_fixed_le<std::int16_t>();
    }

    std::expected<std::uint16_t, ReadError> read_ushort_le() {
        return read_fixed_le<std::uint16_t>();
    }

    std::expected<std::int32_t, ReadError> read_int_le() {
        return read_fixed_le<std::int32_t>();
    }

    std::expected<std::int32_t, ReadError> read_int_be() {
        if (remaining() < 4) return std::unexpected(ReadError::out_of_bounds);
        std::int32_t val;
        std::memcpy(&val, data_.data() + pos_, 4);
        pos_ += 4;
        // Byte-swap from big-endian to native (little-endian on x86)
        auto u = static_cast<std::uint32_t>(val);
        u = ((u >> 24) & 0xFF) | ((u >> 8) & 0xFF00) |
            ((u << 8) & 0xFF0000) | ((u << 24) & 0xFF000000);
        return static_cast<std::int32_t>(u);
    }

    std::expected<std::uint32_t, ReadError> read_uint_le() {
        return read_fixed_le<std::uint32_t>();
    }

    std::expected<std::int64_t, ReadError> read_int64_le() {
        return read_fixed_le<std::int64_t>();
    }

    std::expected<float, ReadError> read_float_le() {
        return read_fixed_le<float>();
    }

    std::expected<double, ReadError> read_double_le() {
        return read_fixed_le<double>();
    }

    std::expected<std::uint32_t, ReadError> read_uvarint() {
        if (pos_ >= data_.size()) return std::unexpected(ReadError::out_of_bounds);
        auto b = static_cast<std::uint8_t>(data_[pos_++]);
        if (b < 0x80) return static_cast<std::uint32_t>(b);

        std::uint32_t result = b & 0x7F;
        int shift = 7;
        while (true) {
            if (pos_ >= data_.size()) return std::unexpected(ReadError::out_of_bounds);
            b = static_cast<std::uint8_t>(data_[pos_++]);
            result |= static_cast<std::uint32_t>(b & 0x7F) << shift;
            if ((b & 0x80) == 0) break;
            shift += 7;
            if (shift >= 35) return std::unexpected(ReadError::varint_too_long);
        }
        return result;
    }

    std::expected<std::int32_t, ReadError> read_varint() {
        auto raw = read_uvarint();
        if (!raw) return std::unexpected(raw.error());
        // Zigzag decode
        return static_cast<std::int32_t>((*raw >> 1) ^ -(static_cast<std::int32_t>(*raw & 1)));
    }

    std::expected<std::uint64_t, ReadError> read_uvarint64() {
        if (pos_ >= data_.size()) return std::unexpected(ReadError::out_of_bounds);
        auto b = static_cast<std::uint8_t>(data_[pos_++]);
        if (b < 0x80) return static_cast<std::uint64_t>(b);

        std::uint64_t result = b & 0x7F;
        int shift = 7;
        while (true) {
            if (pos_ >= data_.size()) return std::unexpected(ReadError::out_of_bounds);
            b = static_cast<std::uint8_t>(data_[pos_++]);
            result |= static_cast<std::uint64_t>(b & 0x7F) << shift;
            if ((b & 0x80) == 0) break;
            shift += 7;
            if (shift >= 70) return std::unexpected(ReadError::varint_too_long);
        }
        return result;
    }

    std::expected<std::int64_t, ReadError> read_varint64() {
        auto raw = read_uvarint64();
        if (!raw) return std::unexpected(raw.error());
        // Zigzag decode
        return static_cast<std::int64_t>((*raw >> 1) ^ -(static_cast<std::int64_t>(*raw & 1)));
    }

    static constexpr std::size_t max_string_bytes = 131068; // 32767 * 4

    std::expected<std::string, ReadError> read_string() {
        auto length = read_uvarint();
        if (!length) return std::unexpected(length.error());
        if (*length > max_string_bytes) return std::unexpected(ReadError::string_too_long);
        auto bytes = read_bytes(*length);
        if (!bytes) return std::unexpected(bytes.error());
        return std::string(bytes->data(), bytes->size());
    }

    // Template read interface for type tags
    template <typename Tag>
    auto read() -> std::expected<typename Tag::value_type, ReadError>;

private:
    template <typename T>
    std::expected<T, ReadError> read_fixed_le() {
        if (remaining() < sizeof(T)) return std::unexpected(ReadError::out_of_bounds);
        T val;
        std::memcpy(&val, data_.data() + pos_, sizeof(T));
        pos_ += sizeof(T);
        return val;
    }

    std::string_view data_;
    std::size_t pos_ = 0;
};

} // namespace endweave
