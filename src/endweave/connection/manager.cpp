#include "endweave/connection/manager.h"

#include <unordered_map>

namespace endweave {

ConnectionManager::ConnectionManager(ProtocolManager &protocol_manager, endstone::Logger &logger,
                                     int server_protocol_version)
    : protocol_manager_(&protocol_manager), logger_(&logger), server_protocol_version_(server_protocol_version)
{
}

UserConnection &ConnectionManager::getOrCreate(const endstone::SocketAddress &address)
{
    auto it = connections_.find(address);
    if (it == connections_.end()) {
        it = connections_.try_emplace(address, *protocol_manager_, *logger_, address, server_protocol_version_).first;
    }
    return it->second;
}

UserConnection *ConnectionManager::get(const endstone::SocketAddress &address)
{
    const auto it = connections_.find(address);
    return it == connections_.end() ? nullptr : &it->second;
}

void ConnectionManager::onDisconnect(const endstone::SocketAddress &address)
{
    connections_.erase(address);
}

void ConnectionManager::sweep(std::chrono::seconds idle_timeout)
{
    const auto deadline = std::chrono::steady_clock::now() - idle_timeout;
    std::erase_if(connections_, [deadline](const auto &entry) {
        return entry.second.getLastSeen() < deadline;
    });
}

} // namespace endweave
