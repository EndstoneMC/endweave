#pragma once

#include "endweave/connection/connection.h"

#include <chrono>
#include <cstddef>
#include <endstone/logger.h>
#include <string>
#include <unordered_map>

namespace endweave {

class ProtocolManager;

/**
 * The live connections, mirroring ViaVersion's ConnectionManager.
 *
 * Keyed by peer address rather than player, because the version handshake happens well before a
 * player object exists.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 */
class ConnectionManager {
public:
    /**
     * Constructs the manager.
     *
     * @param protocol_manager The registry, which must outlive every connection.
     * @param logger The server logger.
     * @param server_protocol_version The version the server itself speaks.
     */
    ConnectionManager(ProtocolManager &protocol_manager, endstone::Logger &logger, int server_protocol_version);

    /**
     * Gets the connection for an address, creating it if this is the first packet.
     *
     * @param address The peer address.
     * @return The connection. The reference stays valid until the connection is removed.
     */
    UserConnection &getOrCreate(const std::string &address);

    /**
     * Looks up an existing connection.
     *
     * @param address The peer address.
     * @return The connection, or nullptr if there is none.
     */
    [[nodiscard]] UserConnection *get(const std::string &address);

    /**
     * Drops a connection that has gone away.
     *
     * @param address The peer address.
     */
    void onDisconnect(const std::string &address);

    /**
     * Drops connections that have carried no packet for a while.
     *
     * A connection that fails before login never produces a quit event, so the table would
     * otherwise grow without bound.
     *
     * @param idle_timeout How long a connection may stay silent before it is dropped.
     */
    void sweep(std::chrono::seconds idle_timeout);

    /**
     * Gets how many connections are being tracked.
     *
     * @return The connection count.
     */
    [[nodiscard]] std::size_t size() const
    {
        return connections_.size();
    }

private:
    ProtocolManager *protocol_manager_;
    endstone::Logger *logger_;
    int server_protocol_version_;
    // Node-based, so a reference handed to a handler survives later insertions.
    std::unordered_map<std::string, UserConnection> connections_;
};

} // namespace endweave
