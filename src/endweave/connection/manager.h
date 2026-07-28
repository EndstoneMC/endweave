#pragma once

#include "endweave/connection/connection.h"

#include <chrono>
#include <endstone/endstone.hpp>
#include <unordered_map>

namespace endweave {

class ProtocolManager;

/**
 * The live connections, keyed by peer address because the version handshake happens before a
 * player object exists.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 *
 * @see ViaVersion ConnectionManager (api) and ConnectionManagerImpl (common). ViaVersion keys
 * UUID maps once login succeeds. endweave owns each connection from its first packet, by address.
 */
class ConnectionManager {
public:
    /**
     * @param protocol_manager The registry, which must outlive every connection.
     * @param logger The server logger.
     * @param server_protocol_version The version the server itself speaks.
     */
    ConnectionManager(ProtocolManager &protocol_manager, endstone::Logger &logger, int server_protocol_version);

    /**
     * Gets the connection for an address, creating it if this is the first packet.
     *
     * @note endweave-specific: ViaVersion registers connections in onLoginSuccess, not lazily.
     */
    UserConnection &getOrCreate(const endstone::SocketAddress &address);

    /**
     * @return The connection for an address, or nullptr if there is none.
     * @see ViaVersion ConnectionManager#getServerConnection(UUID).
     */
    [[nodiscard]] UserConnection *get(const endstone::SocketAddress &address);

    /** @see ViaVersion ConnectionManager#onDisconnect(UserConnection). */
    void onDisconnect(const endstone::SocketAddress &address);

    /**
     * Drops connections that have carried no packet for a while.
     *
     * @param idle_timeout How long a connection may stay silent before it is dropped.
     * @note endweave-specific: ViaVersion evicts on the netty channel-close future.
     */
    void sweep(std::chrono::seconds idle_timeout);

private:
    ProtocolManager *protocol_manager_; // endweave-specific
    endstone::Logger *logger_;          // endweave-specific
    int server_protocol_version_;       // endweave-specific
    // ViaVersion: serverConnections + clientConnections (by UUID). endweave keeps one
    // address-keyed, node-based map, so references handed to handlers survive later insertions.
    std::unordered_map<endstone::SocketAddress, UserConnection> connections_;
};

} // namespace endweave
