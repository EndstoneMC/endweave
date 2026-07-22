#pragma once

#include "endweave/protocol/path.h"
#include "endweave/protocol/protocol.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

namespace endweave {

/**
 * The version graph and its path finding, mirroring ViaVersion's ProtocolManager.
 *
 * The vertices are protocols -- one Protocol<V> per protocol version, owned here -- and the
 * edges join a node to the node registered before it. Both directions of an edge are served by
 * the newer of its two nodes, so registering a node is all it takes to wire the step up.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 */
class ProtocolManager {
public:
    /**
     * Constructs and owns Protocol<V>, initialises it, inserts the vertex, and links it to
     * the previously registered node in both directions.
     *
     * @note A node's predecessor is whichever node was registered before it, so protocols must
     * be registered in ascending version order. Throws std::invalid_argument otherwise.
     */
    template <ProtocolVersion V>
    void registerProtocol()
    {
        registerProtocol(std::make_unique<Protocol<V>>());
    }

    /**
     * Registers a base protocol, which sits at the head of every pipeline and is not a vertex.
     *
     * @param base_protocol The protocol. Throws std::invalid_argument if it is not a base one.
     */
    void registerBaseProtocol(std::unique_ptr<AbstractProtocol> base_protocol);

    /**
     * ViaVersion's registerProtocols(): the one place the version chain is listed, in
     * ascending order.
     */
    void registerProtocols();

    /**
     * Looks up the node for a protocol version.
     *
     * @param version The protocol version.
     * @return The node, or nullptr if that version is not registered.
     */
    [[nodiscard]] const AbstractProtocol *getProtocol(int version) const;

    /**
     * Gets the registered base protocols, in registration order.
     *
     * @return The base protocols.
     */
    [[nodiscard]] const std::vector<const AbstractProtocol *> &getBaseProtocols() const
    {
        return base_protocols_;
    }

    /**
     * Finds the shortest path from a client version to a server version.
     *
     * Breadth-first over the node graph. The result is in serverbound order and is cached
     * until the next registerProtocol(), including the "no path" answer.
     *
     * @param client_protocol_version The version the stream starts in.
     * @param server_protocol_version The version the stream must end in.
     * @return The path, empty if the two versions are equal, or std::nullopt if unreachable.
     */
    [[nodiscard]] std::optional<ProtocolPath> getProtocolPath(int client_protocol_version, int server_protocol_version);

    /**
     * Gets the client versions this server can serve.
     *
     * @return The supported versions, ascending.
     */
    [[nodiscard]] const std::set<int> &getSupportedVersions() const
    {
        return supported_versions_;
    }

    /**
     * Recomputes the supported version set for a server version.
     *
     * @param server_protocol_version The version the server itself speaks.
     */
    void refreshVersions(int server_protocol_version);

    /**
     * Gets the fail-safe cap on how many hops a path may contain.
     *
     * @return The maximum path length.
     */
    [[nodiscard]] int getMaxProtocolPathSize() const
    {
        return max_protocol_path_size_;
    }

private:
    /**
     * One direction of an edge: where it leads, and which table serves it.
     */
    struct Edge {
        int to = 0;
        const AbstractProtocol *protocol = nullptr;
        Step step = Step::Upgrade;
    };

    /**
     * A vertex: the node's protocol and the edges leaving it.
     */
    struct Node {
        const AbstractProtocol *protocol = nullptr;
        std::vector<Edge> edges;
    };

    void registerProtocol(std::unique_ptr<AbstractProtocol> protocol);
    [[nodiscard]] std::optional<ProtocolPath> calculateProtocolPath(int from, int to) const;

    std::vector<std::unique_ptr<AbstractProtocol>> owned_;
    std::vector<std::unique_ptr<AbstractProtocol>> owned_base_;
    // Sorted, so the previously registered node is the last entry.
    std::map<int, Node> nodes_;
    std::vector<const AbstractProtocol *> base_protocols_;
    std::unordered_map<ProtocolPathKey, std::optional<ProtocolPath>, ProtocolPathKeyHash> path_cache_;
    std::set<int> supported_versions_;
    int max_protocol_path_size_ = 50;
};

} // namespace endweave
