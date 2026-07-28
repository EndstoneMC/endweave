#pragma once

#include "endweave/protocol/error.h"

#include <any>
#include <bedrock/serializer.hpp>
#include <bedrock/stream.hpp>
#include <expected>
#include <string_view>
#include <utility>

namespace endweave {

/**
 * A packet travelling through a pipeline: the undecoded body until a stage asks for it as a
 * type, the decoded struct from then on. Type-erased, because the struct a body decodes to
 * depends on the version the connection speaks.
 *
 * @note endweave-specific: what ViaVersion's PacketWrapper carries. ViaVersion reads an untyped
 * buffer field by field through its Type<T> registry, where bedrock-protocol decodes a whole
 * body into a generated struct, so the carrier holds one value instead of a value list.
 */
class PacketHolder {
public:
    /**
     * @param payload The packet body, excluding the header.
     */
    explicit PacketHolder(std::string_view payload) : payload_(payload) {}

    /**
     * Reads the packet as P, decoding the body on first use.
     *
     * @return The packet, or why it could not be read. A body that leaves bytes unread is
     * PacketError::TrailingBytes, and asking for a type other than the one a previous stage left
     * behind is PacketError::NoConverter: the step that reshaped this packet registered none.
     */
    template <class P>
    [[nodiscard]] std::expected<const P *, PacketError> get()
    {
        if (!value_.has_value()) {
            bedrock::protocol::BinaryReader reader{payload_};
            auto decoded = bedrock::protocol::Serializer<P>::deserialize(reader);
            if (!decoded) {
                return std::unexpected(PacketError::Malformed);
            }
            if (reader.getUnreadLength() != 0) {
                return std::unexpected(PacketError::TrailingBytes);
            }
            set(std::move(decoded.value()));
        }
        const P *packet = std::any_cast<P>(&value_);
        if (packet == nullptr) {
            return std::unexpected(PacketError::NoConverter);
        }
        return packet;
    }

    /** Leaves the next version's shape of the packet behind for the stages after this one. */
    template <class P>
    void set(P packet)
    {
        serialize_ = [](bedrock::protocol::BinaryWriter &out, const std::any &value) {
            bedrock::protocol::Serializer<P>::serialize(out, *std::any_cast<P>(&value));
        };
        value_ = std::move(packet);
    }

    /** @return true once a stage has decoded the body. */
    [[nodiscard]] bool isDecoded() const
    {
        return value_.has_value();
    }

    /** Writes the packet back out. The caller checks isDecoded() first. */
    void serialize(bedrock::protocol::BinaryWriter &out) const
    {
        serialize_(out, value_);
    }

private:
    std::string_view payload_;
    std::any value_;
    void (*serialize_)(bedrock::protocol::BinaryWriter &, const std::any &) = nullptr;
};

} // namespace endweave
