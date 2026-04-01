#pragma once

/// PacketWrapper - read/transform/write packet manipulation.
///
/// Holds separate read (input) and write (output) buffers. Handlers use
/// template operations to transform packets field-by-field:
///
///     wrapper.passthrough<var_int>()         -- read + write, return value
///     wrapper.read<var_int>()                -- read only (removes field)
///     wrapper.write<var_int>(42)             -- write only (inserts field)
///     wrapper.map<network_block_pos, block_pos>()  -- read as one type, write as other
///
/// After all handlers run, to_string() produces the final payload from the
/// write buffer plus any unread trailing bytes.
///
/// See Also:
///     com.viaversion.viaversion.api.type.Types

#include <expected>
#include <string>
#include <string_view>

#include "endweave/codec/reader.h"
#include "endweave/codec/writer.h"

namespace endweave {

// Forward declaration
class UserConnection;

class PacketWrapper {
public:
    explicit PacketWrapper(std::string_view payload, UserConnection* user = nullptr)
        : reader_(payload), user_(user) {}

    [[nodiscard]] UserConnection& user() const { return *user_; }
    [[nodiscard]] bool has_user() const { return user_ != nullptr; }
    [[nodiscard]] PacketReader& reader() { return reader_; }
    [[nodiscard]] PacketWriter& writer() { return writer_; }
    [[nodiscard]] bool cancelled() const { return cancelled_; }
    void cancel() { cancelled_ = true; }
    [[nodiscard]] bool has_remaining() const { return reader_.has_remaining(); }

    /// Read a field from input and write it to output.
    template <typename Tag>
    auto passthrough() -> std::expected<typename Tag::value_type, ReadError> {
        auto val = reader_.read<Tag>();
        if (!val) return std::unexpected(val.error());
        writer_.write<Tag>(*val);
        return val;
    }

    /// Read a field from input without writing (removes field from output).
    template <typename Tag>
    auto read() -> std::expected<typename Tag::value_type, ReadError> {
        return reader_.read<Tag>();
    }

    /// Write a field to output without reading (inserts a new field).
    template <typename Tag>
    void write(const typename Tag::value_type& value) {
        writer_.write<Tag>(value);
    }

    /// Read a field with one type tag and write it with another.
    template <typename OldTag, typename NewTag>
    auto map() -> std::expected<typename OldTag::value_type, ReadError> {
        auto val = reader_.read<OldTag>();
        if (!val) return std::unexpected(val.error());
        writer_.write<NewTag>(*val);
        return val;
    }

    /// Copy all remaining input bytes to output.
    std::string_view passthrough_all() {
        auto remaining = reader_.read_remaining();
        writer_.write_bytes(remaining);
        return remaining;
    }

    /// Produce final payload: written output + any unread input bytes.
    std::string to_string() {
        if (reader_.has_remaining()) {
            writer_.write_bytes(reader_.read_remaining());
        }
        return writer_.to_string();
    }

private:
    PacketReader reader_;
    PacketWriter writer_;
    bool cancelled_ = false;
    UserConnection* user_;
};

} // namespace endweave
