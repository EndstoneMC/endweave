#pragma once

/// Binary packet writer for Bedrock protocol serialization.
///
/// Accumulates bytes into a vector<uint8_t> buffer. All writes are infallible
/// (just grow the buffer).

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace endweave {

class PacketWriter {
public:
    PacketWriter() { buf_.reserve(256); }

    [[nodiscard]] std::string_view to_string_view() const {
        return {reinterpret_cast<const char*>(buf_.data()), buf_.size()};
    }

    [[nodiscard]] std::string to_string() const {
        return {reinterpret_cast<const char*>(buf_.data()), buf_.size()};
    }

    [[nodiscard]] std::size_t size() const { return buf_.size(); }

    void write_byte(std::uint8_t val) {
        buf_.push_back(val);
    }

    void write_bytes(std::string_view data) {
        buf_.insert(buf_.end(),
                    reinterpret_cast<const std::uint8_t*>(data.data()),
                    reinterpret_cast<const std::uint8_t*>(data.data() + data.size()));
    }

    void write_bool(bool val) {
        buf_.push_back(val ? 1 : 0);
    }

    void write_short_le(std::int16_t val) { write_fixed_le(val); }
    void write_ushort_le(std::uint16_t val) { write_fixed_le(val); }
    void write_int_le(std::int32_t val) { write_fixed_le(val); }
    void write_uint_le(std::uint32_t val) { write_fixed_le(val); }
    void write_int64_le(std::int64_t val) { write_fixed_le(val); }
    void write_float_le(float val) { write_fixed_le(val); }
    void write_double_le(double val) { write_fixed_le(val); }

    void write_int_be(std::int32_t val) {
        auto u = static_cast<std::uint32_t>(val);
        u = ((u >> 24) & 0xFF) | ((u >> 8) & 0xFF00) |
            ((u << 8) & 0xFF0000) | ((u << 24) & 0xFF000000);
        write_fixed_le(static_cast<std::int32_t>(u));
    }

    void write_uvarint(std::uint32_t val) {
        if (val < 0x80) {
            buf_.push_back(static_cast<std::uint8_t>(val));
            return;
        }
        while (true) {
            auto byte = static_cast<std::uint8_t>(val & 0x7F);
            val >>= 7;
            if (val) {
                buf_.push_back(byte | 0x80);
            } else {
                buf_.push_back(byte);
                break;
            }
        }
    }

    void write_varint(std::int32_t val) {
        // Zigzag encode
        write_uvarint(static_cast<std::uint32_t>((val << 1) ^ (val >> 31)));
    }

    void write_uvarint64(std::uint64_t val) {
        if (val < 0x80) {
            buf_.push_back(static_cast<std::uint8_t>(val));
            return;
        }
        while (true) {
            auto byte = static_cast<std::uint8_t>(val & 0x7F);
            val >>= 7;
            if (val) {
                buf_.push_back(byte | 0x80);
            } else {
                buf_.push_back(byte);
                break;
            }
        }
    }

    void write_varint64(std::int64_t val) {
        // Zigzag encode
        write_uvarint64(static_cast<std::uint64_t>((val << 1) ^ (val >> 63)));
    }

    void write_string(const std::string& val) {
        write_uvarint(static_cast<std::uint32_t>(val.size()));
        write_bytes(val);
    }

    // Template write interface for type tags
    template <typename Tag>
    void write(const typename Tag::value_type& val);

private:
    template <typename T>
    void write_fixed_le(T val) {
        auto pos = buf_.size();
        buf_.resize(pos + sizeof(T));
        std::memcpy(buf_.data() + pos, &val, sizeof(T));
    }

    std::vector<std::uint8_t> buf_;
};

} // namespace endweave
