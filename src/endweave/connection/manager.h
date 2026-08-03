#pragma once

#include "endweave/connection/connection.h"

#include <chrono>
#include <endstone/endstone.hpp>
#include <unordered_map>

namespace endweave {

/** @see ViaVersion ConnectionManager (api) and ConnectionManagerImpl (common). */
class ConnectionManager {
public:
    UserConnection &getOrCreate(const endstone::SocketAddress &address);

    /** @see ViaVersion ConnectionManager#getServerConnection(UUID). */
    [[nodiscard]] UserConnection *get(const endstone::SocketAddress &address);

    /** @see ViaVersion ConnectionManager#onDisconnect(UserConnection). */
    void onDisconnect(const endstone::SocketAddress &address);

    void sweep(std::chrono::seconds idle_timeout);

private:
    std::unordered_map<endstone::SocketAddress, UserConnection> connections_;
};

} // namespace endweave
