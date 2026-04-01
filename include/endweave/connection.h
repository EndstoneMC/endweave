#pragma once

/// Per-player connection tracking for protocol translation.
///
/// See Also:
///     com.viaversion.viaversion.connection.UserConnectionImpl

#include <any>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace endweave {

class Protocol;

class UserConnection {
public:
    UserConnection(std::string address, int server_protocol)
        : address_(std::move(address))
        , server_protocol_(server_protocol) {}

    [[nodiscard]] const std::string& address() const { return address_; }
    [[nodiscard]] int client_protocol() const { return client_protocol_; }
    [[nodiscard]] int server_protocol() const { return server_protocol_; }
    [[nodiscard]] bool warned_no_chain() const { return warned_no_chain_; }

    void set_client_protocol(int v) { client_protocol_ = v; }
    void set_warned_no_chain(bool v) { warned_no_chain_ = v; }

    [[nodiscard]] bool needs_translation() const {
        return client_protocol_ != 0 && client_protocol_ != server_protocol_;
    }

    // Pipeline cache
    [[nodiscard]] const std::vector<Protocol*>* protocol_pipeline() const {
        return pipeline_set_ ? &pipeline_ : nullptr;
    }
    [[nodiscard]] const std::vector<Protocol*>* clientbound_pipeline() const {
        return cb_pipeline_set_ ? &cb_pipeline_ : nullptr;
    }

    void set_pipeline(std::vector<Protocol*> pipeline) {
        pipeline_ = std::move(pipeline);
        pipeline_set_ = true;
    }
    void set_clientbound_pipeline(std::vector<Protocol*> pipeline) {
        cb_pipeline_ = std::move(pipeline);
        cb_pipeline_set_ = true;
    }

    // Type-keyed storage
    template <typename T>
    T* get() {
        auto it = storage_.find(std::type_index(typeid(T)));
        if (it == storage_.end()) return nullptr;
        return std::any_cast<T>(&it->second);
    }

    template <typename T>
    void put(T value) {
        storage_[std::type_index(typeid(T))] = std::move(value);
    }

    template <typename T>
    bool has() const {
        return storage_.contains(std::type_index(typeid(T)));
    }

    template <typename T>
    void remove() {
        storage_.erase(std::type_index(typeid(T)));
    }

    void clear_storage() { storage_.clear(); }

private:
    std::string address_;
    int client_protocol_ = 0;
    int server_protocol_;
    bool warned_no_chain_ = false;
    bool pipeline_set_ = false;
    bool cb_pipeline_set_ = false;
    std::vector<Protocol*> pipeline_;
    std::vector<Protocol*> cb_pipeline_;
    std::unordered_map<std::type_index, std::any> storage_;
};

class ConnectionManager {
public:
    explicit ConnectionManager(int server_protocol = 0)
        : server_protocol_(server_protocol) {}

    UserConnection& get_or_create(const std::string& address) {
        auto it = connections_.find(address);
        if (it == connections_.end()) {
            it = connections_.emplace(address, UserConnection(address, server_protocol_)).first;
        }
        return it->second;
    }

    UserConnection* get(const std::string& address) {
        auto it = connections_.find(address);
        return it != connections_.end() ? &it->second : nullptr;
    }

    void remove(const std::string& address) {
        auto it = connections_.find(address);
        if (it != connections_.end()) {
            it->second.clear_storage();
            connections_.erase(it);
        }
    }

private:
    int server_protocol_;
    std::unordered_map<std::string, UserConnection> connections_;
};

} // namespace endweave
