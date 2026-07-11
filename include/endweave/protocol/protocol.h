#pragma once

#include "endweave/protocol/direction.h"

#include <bedrock/serializer.hpp>
#include <bedrock/stream.hpp>
#include <expected>
#include <functional>
#include <initializer_list>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace endweave {

class UserConnection;

// Outcome of Protocol::transform. Passthrough is "no handler, not cancelled" (out
// left untouched, caller forwards the original bytes); Cancelled stands in for
// ViaVersion's PacketWrapper::cancel(); Translated means the rewritten body is in out.
enum class TransformResult {
    Passthrough,
    Translated,
    Cancelled
};

// A per-boundary, bidirectional protocol, mirroring ViaVersion's Protocol. It holds
// one handler table and one cancel set per Direction and translates a packet body
// (bytes in -> bytes out) through the bedrock-protocol codec.
class Protocol {
public:
    using Handler = std::function<std::expected<void, std::error_code>(
        UserConnection &, bedrock::protocol::BinaryReader &, bedrock::protocol::BinaryWriter &)>;

    Protocol(int server_protocol, int client_protocol, std::string name = "", bool is_base = false);

    void register_clientbound(int packet_id, Handler handler)
    {
        handlers_[Direction::Clientbound][packet_id] = std::move(handler);
    }

    void register_serverbound(int packet_id, Handler handler)
    {
        handlers_[Direction::Serverbound][packet_id] = std::move(handler);
    }

    void cancel_clientbound(std::initializer_list<int> packet_ids)
    {
        cancel_[Direction::Clientbound].insert(packet_ids);
    }

    void cancel_serverbound(std::initializer_list<int> packet_ids)
    {
        cancel_[Direction::Serverbound].insert(packet_ids);
    }

    [[nodiscard]] bool has_handler_or_cancel(Direction direction, int packet_id) const;

    std::expected<TransformResult, std::error_code> transform(Direction direction, int packet_id,
                                                              UserConnection &connection,
                                                              bedrock::protocol::BinaryReader &in,
                                                              bedrock::protocol::BinaryWriter &out) const;

    // Per-connection init hook (ViaVersion's Protocol.init), called once when a
    // chain is first resolved for a connection. No-op unless set_init installs one.
    void set_init(std::function<void(UserConnection &)> on_init)
    {
        on_init_ = std::move(on_init);
    }
    void init(UserConnection &connection) const
    {
        if (on_init_) {
            on_init_(connection);
        }
    }

    [[nodiscard]] int server_protocol() const
    {
        return server_protocol_;
    }
    [[nodiscard]] int client_protocol() const
    {
        return client_protocol_;
    }
    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }
    [[nodiscard]] bool is_base() const
    {
        return is_base_;
    }

private:
    int server_protocol_;
    int client_protocol_;
    std::string name_;
    bool is_base_;
    std::unordered_map<Direction, std::unordered_map<int, Handler>> handlers_;
    std::unordered_map<Direction, std::unordered_set<int>> cancel_;
    std::function<void(UserConnection &)> on_init_;
};

// Wraps a typed converter as a byte-level Handler: deserialize From, convert,
// serialize To -- all through the bedrock-protocol codec. From/To are deduced.
template <class From, class To>
Protocol::Handler translate(To (*fn)(const From &))
{
    return [fn](UserConnection &, bedrock::protocol::BinaryReader &in,
                bedrock::protocol::BinaryWriter &out) -> std::expected<void, std::error_code> {
        auto packet = bedrock::protocol::Serializer<From>::deserialize(in);
        if (!packet) {
            return std::unexpected(packet.error());
        }
        bedrock::protocol::Serializer<To>::serialize(out, fn(*packet));
        return {};
    };
}

} // namespace endweave
