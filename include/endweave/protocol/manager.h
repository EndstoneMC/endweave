#pragma once

/// Protocol manager with BFS chaining for multi-step version gaps.
///
/// See Also:
///     com.viaversion.viaversion.protocol.ProtocolManagerImpl

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "endweave/protocol/protocol.h"

namespace endweave {

class ProtocolManager {
public:
    void register_protocol(Protocol* protocol) {
        auto key = make_key(protocol->server_protocol(), protocol->client_protocol());
        protocols_[key] = protocol;
        path_cache_.clear();
    }

    void register_base(Protocol* protocol) {
        base_protocols_.push_back(protocol);
    }

    [[nodiscard]] const std::vector<Protocol*>& base_protocols() const {
        return base_protocols_;
    }

    [[nodiscard]] Protocol* get(int server_protocol, int client_protocol) const {
        auto it = protocols_.find(make_key(server_protocol, client_protocol));
        return it != protocols_.end() ? it->second : nullptr;
    }

    /// Return ordered protocol chain, or nullptr if no path exists.
    /// Empty vector means same version (no translation needed).
    const std::vector<Protocol*>* get_path(int server_protocol, int client_protocol) {
        if (server_protocol == client_protocol) {
            static const std::vector<Protocol*> empty;
            return &empty;
        }

        auto key = make_key(server_protocol, client_protocol);
        auto cache_it = path_cache_.find(key);
        if (cache_it != path_cache_.end()) {
            return cache_it->second.has_value() ? &cache_it->second.value() : nullptr;
        }

        // Direct lookup
        auto direct_it = protocols_.find(key);
        if (direct_it != protocols_.end()) {
            auto& cached = path_cache_[key];
            cached = std::vector<Protocol*>{direct_it->second};
            return &cached.value();
        }

        // BFS
        auto path = bfs(server_protocol, client_protocol);
        auto& cached = path_cache_[key];
        if (path) {
            cached = std::move(*path);
            return &cached.value();
        }
        cached = std::nullopt;
        return nullptr;
    }

    /// Return all protocol versions reachable from the given server protocol.
    std::vector<int> get_supported_versions(int server_protocol) {
        std::vector<int> result;
        result.push_back(server_protocol);
        for (auto& [key, _] : protocols_) {
            int client = static_cast<int>(key & 0xFFFFFFFF);
            if (get_path(server_protocol, client) != nullptr) {
                if (std::find(result.begin(), result.end(), client) == result.end()) {
                    result.push_back(client);
                }
            }
        }
        std::sort(result.begin(), result.end());
        return result;
    }

private:
    using Key = std::uint64_t;

    static Key make_key(int server, int client) {
        return (static_cast<Key>(static_cast<unsigned int>(server)) << 32)
             | static_cast<Key>(static_cast<unsigned int>(client));
    }

    std::optional<std::vector<Protocol*>> bfs(int server_protocol, int client_protocol) const {
        // Build adjacency: from client_protocol -> server_protocol via protocol
        std::unordered_map<int, std::vector<Protocol*>> adjacency;
        for (auto& [_, protocol] : protocols_) {
            adjacency[protocol->client_protocol()].push_back(protocol);
        }

        std::unordered_set<int> visited;
        visited.insert(client_protocol);

        std::deque<std::pair<int, std::vector<Protocol*>>> queue;
        queue.emplace_back(client_protocol, std::vector<Protocol*>{});

        while (!queue.empty()) {
            auto [current, path] = std::move(queue.front());
            queue.pop_front();

            auto adj_it = adjacency.find(current);
            if (adj_it == adjacency.end()) continue;

            for (auto* protocol : adj_it->second) {
                int next = protocol->server_protocol();
                if (next == server_protocol) {
                    path.push_back(protocol);
                    return path;
                }
                if (!visited.contains(next)) {
                    visited.insert(next);
                    auto new_path = path;
                    new_path.push_back(protocol);
                    queue.emplace_back(next, std::move(new_path));
                }
            }
        }
        return std::nullopt;
    }

    std::unordered_map<Key, Protocol*> protocols_;
    std::vector<Protocol*> base_protocols_;
    std::unordered_map<Key, std::optional<std::vector<Protocol*>>> path_cache_;
};

} // namespace endweave
