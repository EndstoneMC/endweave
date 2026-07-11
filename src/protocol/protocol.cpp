#include "endweave/protocol/protocol.h"

#include <string>

namespace endweave {

Protocol::Protocol(int server_protocol, int client_protocol, std::string name, bool is_base)
    : server_protocol_(server_protocol), client_protocol_(client_protocol), name_(std::move(name)), is_base_(is_base)
{
    if (name_.empty()) {
        name_ = std::to_string(server_protocol_) + "->" + std::to_string(client_protocol_);
    }
}

bool Protocol::has_handler_or_cancel(Direction direction, int packet_id) const
{
    if (auto c = cancel_.find(direction); c != cancel_.end() && c->second.contains(packet_id)) {
        return true;
    }
    if (auto d = handlers_.find(direction); d != handlers_.end() && d->second.contains(packet_id)) {
        return true;
    }
    return false;
}

std::expected<TransformResult, std::error_code> Protocol::transform(Direction direction, int packet_id,
                                                                    UserConnection &connection,
                                                                    bedrock::protocol::BinaryReader &in,
                                                                    bedrock::protocol::BinaryWriter &out) const
{
    if (auto c = cancel_.find(direction); c != cancel_.end() && c->second.contains(packet_id)) {
        return TransformResult::Cancelled;
    }
    auto d = handlers_.find(direction);
    if (d == handlers_.end()) {
        return TransformResult::Passthrough;
    }
    auto h = d->second.find(packet_id);
    if (h == d->second.end()) {
        return TransformResult::Passthrough;
    }
    if (auto result = h->second(connection, in, out); !result) {
        return std::unexpected(result.error());
    }
    return TransformResult::Translated;
}

} // namespace endweave
