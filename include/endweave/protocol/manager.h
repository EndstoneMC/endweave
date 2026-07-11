#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace endweave {

class Protocol;

// Registry of version protocols + base protocols, mirroring ViaVersion's
// ProtocolManager. Resolves a translation chain between a server and client
// version via BFS over registered protocols, with a path cache. Holds non-owning
// pointers -- the owner (the plugin) must outlive the manager and connections.
class ProtocolManager {
public:
    // `register` is a reserved word in C++, hence register_protocol.
    void register_protocol(const Protocol &protocol);
    void register_base(const Protocol &protocol);
    [[nodiscard]] const std::vector<const Protocol *> &base_protocols() const
    {
        return base_protocols_;
    }

    [[nodiscard]] const Protocol *get(int server_protocol, int client_protocol) const;

    // Empty vector = same version; nullopt = no chain (unreachable). The result
    // (including nullopt) is cached until the next register_protocol.
    std::optional<std::vector<const Protocol *>> get_path(int server_protocol, int client_protocol);

    std::vector<int> get_supported_versions(int server_protocol);

private:
    [[nodiscard]] std::optional<std::vector<const Protocol *>> bfs(int server_protocol, int client_protocol) const;

    struct PairHash {
        std::size_t operator()(const std::pair<int, int> &p) const noexcept
        {
            return std::hash<long long>{}((static_cast<long long>(p.first) << 32) ^ static_cast<unsigned>(p.second));
        }
    };

    std::unordered_map<std::pair<int, int>, const Protocol *, PairHash> protocols_;
    std::vector<const Protocol *> base_protocols_;
    std::unordered_map<std::pair<int, int>, std::optional<std::vector<const Protocol *>>, PairHash> path_cache_;
};

} // namespace endweave
