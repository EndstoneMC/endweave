#pragma once

#include "endweave/protocol/path.h"
#include "endweave/protocol/protocol.h"

#include <bitset>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

namespace endweave {

/**
 * The version graph and its path finding. Vertices are nodes (one Protocol<V> per version,
 * owned here). Edges join a node to the node registered before it.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 *
 * @see ViaVersion ProtocolManager (api) and ProtocolManagerImpl (common).
 */
class ProtocolManager {
public:
    /**
     * Constructs, initialises, and links Protocol<V> into the graph. Protocols must be
     * registered in ascending version order, or it throws std::invalid_argument.
     *
     * @see ViaVersion ProtocolManagerImpl#registerProtocol.
     */
    template <ProtocolVersion V>
    void registerProtocol()
    {
        registerProtocol(std::make_unique<Protocol<V>>());
    }

    /**
     * Registers a base protocol. Throws std::invalid_argument if it is not a base one.
     *
     * @see ViaVersion ProtocolManagerImpl#registerBaseProtocol.
     */
    void registerBaseProtocol(std::unique_ptr<AbstractProtocol> base_protocol);

    /**
     * The one place the version chain is listed, in ascending order.
     *
     * @see ViaVersion ProtocolManagerImpl#registerProtocols.
     */
    void registerProtocols();

    /**
     * @param version The protocol version.
     * @return The node for a version, or nullptr if not registered.
     * @see ViaVersion ProtocolManager#getProtocol.
     */
    [[nodiscard]] const AbstractProtocol *getProtocol(int version) const;

    /**
     * @return The registered base protocols, in registration order.
     * @see ViaVersion ProtocolManager#getBaseProtocol / #getBaseProtocols.
     */
    [[nodiscard]] const std::vector<const AbstractProtocol *> &getBaseProtocols() const
    {
        return base_protocols_;
    }

    /**
     * Finds the shortest path from a client version to a server version, breadth-first. The
     * result is in serverbound order and cached until the next registerProtocol(), "no path"
     * included.
     *
     * @param client_protocol_version The version the stream starts in.
     * @param server_protocol_version The version the stream must end in.
     * @return The path, empty if the versions are equal, or std::nullopt if unreachable.
     * @see ViaVersion ProtocolManagerImpl#getProtocolPath.
     */
    [[nodiscard]] std::optional<ProtocolPath> getProtocolPath(int client_protocol_version, int server_protocol_version);

    /**
     * @return The client versions this server can serve, ascending.
     * @see ViaVersion ProtocolManager#getSupportedVersions.
     */
    [[nodiscard]] const std::set<int> &getSupportedVersions() const
    {
        return supported_versions_;
    }

    /**
     * Recomputes the supported version set for a server version.
     *
     * @see ViaVersion ProtocolManagerImpl#refreshVersions.
     */
    void refreshVersions(int server_protocol_version);

    /**
     * @return The fail-safe cap on how many hops a path may contain.
     * @see ViaVersion ProtocolManager#getMaxProtocolPathSize.
     */
    [[nodiscard]] int getMaxProtocolPathSize() const
    {
        return max_protocol_path_size_;
    }

    /**
     * @param packet_id The packet id.
     * @return true if any registered protocol has a handler for it, at any version.
     * @note endweave-specific. ViaVersion reaches a mapping table only once it holds a connection
     * and its pipeline, so it has nothing to ask before that.
     */
    [[nodiscard]] bool isInteresting(int packet_id) const
    {
        return packet_id >= 0 && packet_id < kPacketIdCount && interesting_.test(static_cast<std::size_t>(packet_id));
    }

private:
    /** One direction of an edge: where it leads, and which table serves it. */
    struct Edge {
        int to = 0;
        const AbstractProtocol *protocol = nullptr;
        Step step = Step::Upgrade;
    };

    /** A vertex: the node's protocol and the edges leaving it. */
    struct Node {
        const AbstractProtocol *protocol = nullptr;
        std::vector<Edge> edges;
    };

    void registerProtocol(std::unique_ptr<AbstractProtocol> protocol);
    /** @see ViaVersion ProtocolManagerImpl#calculateProtocolPath. */
    [[nodiscard]] std::optional<ProtocolPath> calculateProtocolPath(int from, int to) const;
    /** @note endweave-specific, alongside isInteresting. */
    void refreshInteresting();

    // ViaVersion owns nodes via its `protocols` map. endweave owns them by unique_ptr.
    std::vector<std::unique_ptr<AbstractProtocol>> owned_;
    std::vector<std::unique_ptr<AbstractProtocol>> owned_base_;
    std::map<int, Node> nodes_;                            // ViaVersion: registryMap
    std::vector<const AbstractProtocol *> base_protocols_; // ViaVersion: serverbound/clientboundBaseProtocols
    std::unordered_map<ProtocolPathKey, std::optional<ProtocolPath>, ProtocolPathKeyHash> path_cache_; // ViaVersion: pathCache
    std::set<int> supported_versions_; // ViaVersion: supportedVersions
    int max_protocol_path_size_ = 50;  // ViaVersion: maxProtocolPathSize
    // endweave-specific: the union of every registered protocol's tables, for isInteresting.
    std::bitset<kPacketIdCount> interesting_;
};

} // namespace endweave
