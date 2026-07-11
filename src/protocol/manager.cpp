#include "endweave/protocol/manager.h"

#include "endweave/protocol/protocol.h"

#include <deque>
#include <set>
#include <unordered_set>
#include <utility>

namespace endweave {

void ProtocolManager::register_protocol(const Protocol &protocol)
{
    protocols_[{protocol.server_protocol(), protocol.client_protocol()}] = &protocol;
    path_cache_.clear();
}

void ProtocolManager::register_base(const Protocol &protocol)
{
    base_protocols_.push_back(&protocol);
}

const Protocol *ProtocolManager::get(int server_protocol, int client_protocol) const
{
    auto it = protocols_.find({server_protocol, client_protocol});
    return it == protocols_.end() ? nullptr : it->second;
}

std::optional<std::vector<const Protocol *>> ProtocolManager::get_path(int server_protocol, int client_protocol)
{
    if (server_protocol == client_protocol) {
        return std::vector<const Protocol *>{};
    }
    const std::pair<int, int> key{server_protocol, client_protocol};
    if (auto c = path_cache_.find(key); c != path_cache_.end()) {
        return c->second;
    }
    if (const Protocol *direct = get(server_protocol, client_protocol)) {
        std::vector<const Protocol *> result{direct};
        path_cache_[key] = result;
        return result;
    }
    auto path = bfs(server_protocol, client_protocol);
    path_cache_[key] = path; // caches nullopt (unreachable) too
    return path;
}

std::optional<std::vector<const Protocol *>> ProtocolManager::bfs(int server_protocol, int client_protocol) const
{
    // Adjacency indexed by each protocol's own client_protocol; a node (a version)
    // is expanded via every protocol whose client_protocol == that version.
    std::unordered_map<int, std::vector<const Protocol *>> adjacency;
    for (const auto &[key, proto] : protocols_) {
        adjacency[proto->client_protocol()].push_back(proto);
    }

    std::unordered_set<int> visited{client_protocol};
    std::deque<std::pair<int, std::vector<const Protocol *>>> queue;
    queue.push_back({client_protocol, {}});

    while (!queue.empty()) {
        auto [current, path] = queue.front();
        queue.pop_front();
        auto edges = adjacency.find(current);
        if (edges == adjacency.end()) {
            continue;
        }
        for (const Protocol *proto : edges->second) {
            const int next = proto->server_protocol(); // advance toward the server version
            std::vector<const Protocol *> next_path = path;
            next_path.push_back(proto);
            if (next == server_protocol) {
                return next_path; // shortest chain, in serverbound apply order
            }
            if (!visited.contains(next)) {
                visited.insert(next);
                queue.push_back({next, std::move(next_path)});
            }
        }
    }
    return std::nullopt;
}

std::vector<int> ProtocolManager::get_supported_versions(int server_protocol)
{
    std::set<int> supported{server_protocol};
    std::vector<int> clients;
    for (const auto &[key, proto] : protocols_) {
        clients.push_back(key.second);
    }
    for (const int client : clients) {
        if (get_path(server_protocol, client).has_value()) {
            supported.insert(client);
        }
    }
    return {supported.begin(), supported.end()};
}

} // namespace endweave
