#include "endweave/connection.h"

namespace endweave {

UserConnection &ConnectionManager::get_or_create(const std::string &address)
{
    auto it = connections_.find(address);
    if (it == connections_.end()) {
        it = connections_.try_emplace(address, address, *logger_, server_protocol_).first;
    }
    return it->second;
}

UserConnection *ConnectionManager::get(const std::string &address)
{
    auto it = connections_.find(address);
    return it == connections_.end() ? nullptr : &it->second;
}

void ConnectionManager::remove_by_address(const std::string &address)
{
    auto it = connections_.find(address);
    if (it != connections_.end()) {
        it->second.clear_storage();
        connections_.erase(it);
    }
}

} // namespace endweave
