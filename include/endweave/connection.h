#pragma once

#include "endweave/log_sink.h"

#include <any>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace endweave {

class Protocol;

// Per-connection translation state, mirroring ViaVersion's UserConnection. Keyed
// by address; carries the detected client/server protocols, the cached pipelines,
// and a type-keyed store for connection-stateful handlers.
class UserConnection {
public:
    UserConnection(std::string address, LogSink &log, int server_protocol)
        : address_(std::move(address)), log_(&log), server_protocol_(server_protocol)
    {
    }

    [[nodiscard]] const std::string &address() const
    {
        return address_;
    }
    [[nodiscard]] LogSink &log() const
    {
        return *log_;
    }

    [[nodiscard]] int client_protocol() const
    {
        return client_protocol_;
    }
    void set_client_protocol(int protocol)
    {
        client_protocol_ = protocol;
    }
    [[nodiscard]] int server_protocol() const
    {
        return server_protocol_;
    }

    // The client speaks a different, known version than the server.
    [[nodiscard]] bool needs_translation() const
    {
        return client_protocol_ != 0 && client_protocol_ != server_protocol_;
    }

    [[nodiscard]] bool warned_no_chain() const
    {
        return warned_no_chain_;
    }
    void set_warned_no_chain(bool warned)
    {
        warned_no_chain_ = warned;
    }

    // nullopt = not yet resolved; an empty/base-only vector is a resolved state.
    [[nodiscard]] const std::optional<std::vector<const Protocol *>> &protocol_pipeline() const
    {
        return protocol_pipeline_;
    }
    [[nodiscard]] const std::optional<std::vector<const Protocol *>> &clientbound_pipeline() const
    {
        return clientbound_pipeline_;
    }
    void set_pipelines(std::vector<const Protocol *> serverbound,
                       std::optional<std::vector<const Protocol *>> clientbound)
    {
        protocol_pipeline_ = std::move(serverbound);
        clientbound_pipeline_ = std::move(clientbound);
    }

    // Type-keyed storage (ViaVersion's StorableObject pattern).
    template <class T>
    [[nodiscard]] T *get()
    {
        auto it = storage_.find(std::type_index(typeid(T)));
        return it == storage_.end() ? nullptr : std::any_cast<T>(&it->second);
    }
    template <class T>
    void put(T value)
    {
        storage_[std::type_index(typeid(T))] = std::move(value);
    }
    template <class T>
    [[nodiscard]] bool has() const
    {
        return storage_.contains(std::type_index(typeid(T)));
    }
    template <class T>
    void remove()
    {
        storage_.erase(std::type_index(typeid(T)));
    }
    void clear_storage()
    {
        storage_.clear();
    }

private:
    std::string address_;
    LogSink *log_;
    int client_protocol_ = 0;
    int server_protocol_ = 0;
    bool warned_no_chain_ = false;
    std::optional<std::vector<const Protocol *>> protocol_pipeline_;
    std::optional<std::vector<const Protocol *>> clientbound_pipeline_;
    std::unordered_map<std::type_index, std::any> storage_;
};

// Owns the live UserConnections, keyed by address string. std::unordered_map is
// node-based, so held references stay valid across inserts.
class ConnectionManager {
public:
    ConnectionManager(int server_protocol, LogSink &logger) : server_protocol_(server_protocol), logger_(&logger) {}

    UserConnection &get_or_create(const std::string &address);
    [[nodiscard]] UserConnection *get(const std::string &address);
    void remove_by_address(const std::string &address);

private:
    int server_protocol_;
    LogSink *logger_;
    std::unordered_map<std::string, UserConnection> connections_;
};

} // namespace endweave
