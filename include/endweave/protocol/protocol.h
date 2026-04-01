#pragma once

/// Protocol translation class.
///
/// Translates packets between two protocol versions. Handlers are registered
/// per packet ID and direction. Unregistered packets pass through unchanged.
///
/// See Also:
///     com.viaversion.viaversion.api.protocol.Protocol

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "endweave/codec/wrapper.h"
#include "endweave/protocol/direction.h"

namespace endweave {

class UserConnection;

using PacketHandler = void(*)(PacketWrapper&);

class Protocol {
public:
    Protocol(int server_protocol, int client_protocol,
             std::string name = "", bool is_base = false)
        : server_protocol_(server_protocol)
        , client_protocol_(client_protocol)
        , name_(name.empty() ? std::to_string(server_protocol) + "->" + std::to_string(client_protocol) : std::move(name))
        , is_base_(is_base) {}

    [[nodiscard]] int server_protocol() const { return server_protocol_; }
    [[nodiscard]] int client_protocol() const { return client_protocol_; }
    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] bool is_base() const { return is_base_; }

    void register_clientbound(int packet_id, PacketHandler handler) {
        handlers_[idx(Direction::CLIENTBOUND)][packet_id] = handler;
    }

    void register_serverbound(int packet_id, PacketHandler handler) {
        handlers_[idx(Direction::SERVERBOUND)][packet_id] = handler;
    }

    void cancel_clientbound(std::initializer_list<int> ids) {
        for (auto id : ids) cancel_[idx(Direction::CLIENTBOUND)].insert(id);
    }

    void cancel_serverbound(std::initializer_list<int> ids) {
        for (auto id : ids) cancel_[idx(Direction::SERVERBOUND)].insert(id);
    }

    /// Called once per connection when the translation chain is first resolved.
    virtual void init(UserConnection& /*connection*/) {}

    [[nodiscard]] bool has_handler_or_cancel(Direction dir, int packet_id) const {
        auto i = idx(dir);
        return handlers_[i].contains(packet_id) || cancel_[i].contains(packet_id);
    }

    void transform(Direction dir, int packet_id, PacketWrapper& wrapper) const {
        auto i = idx(dir);
        if (cancel_[i].contains(packet_id)) {
            wrapper.cancel();
            return;
        }
        auto it = handlers_[i].find(packet_id);
        if (it != handlers_[i].end()) {
            it->second(wrapper);
        }
    }

    virtual ~Protocol() = default;

private:
    static constexpr int idx(Direction d) {
        return d == Direction::CLIENTBOUND ? 0 : 1;
    }

    int server_protocol_;
    int client_protocol_;
    std::string name_;
    bool is_base_;
    std::unordered_map<int, PacketHandler> handlers_[2]; // [CLIENTBOUND, SERVERBOUND]
    std::unordered_set<int> cancel_[2];
};

} // namespace endweave
