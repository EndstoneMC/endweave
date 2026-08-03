#include "endweave/connection/manager.h"

namespace endweave {

UserConnection &ConnectionManager::getOrCreate(const endstone::SocketAddress &address)
{
    auto it = connections_.find(address);
    if (it == connections_.end()) {
        it = connections_.try_emplace(address, address).first;
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
    std::erase_if(connections_, [deadline](auto &entry) {
        return entry.second.getLastSeen() < deadline;
    });
}

} // namespace endweave
