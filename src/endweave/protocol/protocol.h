#pragma once

#include "endweave/protocol/direction.h"
#include "endweave/protocol/handler.h"
#include "endweave/protocol/packet_ids.h"

#include <array>
#include <bedrock/protocol.hpp>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace endweave {

using bedrock::protocol::ProtocolVersion;

class ProtocolManager;
class UserConnection;

/**
 * Shared protocol machinery, mirroring ViaVersion's AbstractProtocol: two packet-id-keyed
 * handler tables plus the transform that runs one packet body through them.
 *
 * A base protocol addresses the two tables as a transport Direction, a version node as a
 * version Step; a protocol only ever uses one of the two vocabularies.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 */
class AbstractProtocol {
public:
    virtual ~AbstractProtocol() = default;

    AbstractProtocol(const AbstractProtocol &) = delete;
    AbstractProtocol &operator=(const AbstractProtocol &) = delete;

    /**
     * Runs registerPackets() exactly once. Throws std::logic_error if called again.
     */
    void initialize();

    /**
     * Sets up per-connection state for this protocol.
     *
     * @note May be called more than once for a single UserConnection.
     *
     * @param connection The connection being initialised.
     */
    virtual void init(UserConnection &connection)
    {
        (void)connection;
    }

    /**
     * Gets the protocol version this node speaks.
     *
     * @return The protocol version, or std::nullopt for a base protocol.
     */
    [[nodiscard]] std::optional<int> getVersion() const
    {
        return version_;
    }

    /**
     * Gets the version this node steps down to.
     *
     * @note Set by ProtocolManager::registerProtocol from the previously registered node.
     *
     * @return The predecessor's version, or std::nullopt for the chain root and base protocols.
     */
    [[nodiscard]] std::optional<int> getPreviousVersion() const
    {
        return previous_;
    }

    /**
     * Returns whether this protocol is a base protocol.
     *
     * @return true if it stays at the head of every pipeline and does no version translation.
     */
    [[nodiscard]] virtual bool isBaseProtocol() const
    {
        return false;
    }

    /**
     * Gets the display name of this protocol.
     *
     * @return The version number for a node ("1001"), or "base".
     */
    [[nodiscard]] const std::string &getName() const
    {
        return name_;
    }

    /**
     * Checks whether a packet id has a handler in the given table.
     *
     * @param slot The table to look in, from slotOf(Direction) or slotOf(Step).
     * @param packet_id The packet id.
     * @return true if a handler is registered.
     */
    [[nodiscard]] bool hasMapping(std::size_t slot, int packet_id) const;

    /**
     * Runs a packet body through this protocol's handler for the given id.
     *
     * @note An unmapped id must not reach here; the caller checks hasMapping() first.
     *
     * @param slot The table to use, from slotOf(Direction) or slotOf(Step).
     * @param packet_id The packet id.
     * @param connection The connection the packet belongs to.
     * @param in The source-version body.
     * @param out Receives the target-version body.
     * @return Whether the packet was translated or cancelled, or the codec's error.
     */
    [[nodiscard]] std::expected<PacketAction, std::error_code> transform(std::size_t slot, int packet_id,
                                                                         UserConnection &connection,
                                                                         bedrock::protocol::BinaryReader &in,
                                                                         bedrock::protocol::BinaryWriter &out) const;

protected:
    /**
     * Constructs a base protocol, which has no version pair.
     *
     * @param name The display name.
     */
    explicit AbstractProtocol(std::string name);

    /**
     * Constructs a version node, named for its version.
     *
     * @param version The protocol version this node speaks.
     */
    explicit AbstractProtocol(ProtocolVersion version);

    /**
     * Registers the packet handlers for this protocol. To be overridden.
     */
    virtual void registerPackets() {}

    // --- version-step axis: a Protocol<V> node ---

    /** Registers a handler translating this node's predecessor form into its own. */
    void registerUpgrade(MinecraftPacketIds packet_id, PacketHandler handler);
    /** Registers a handler translating this node's form into its predecessor's. */
    void registerDowngrade(MinecraftPacketIds packet_id, PacketHandler handler);
    /** Drops a packet that has no form at this node's version. */
    void cancelUpgrade(MinecraftPacketIds packet_id);
    /** Drops a packet that has no form at the predecessor's version. */
    void cancelDowngrade(MinecraftPacketIds packet_id);
    /** Adds to whatever upgrade handler is registered, or registers if none is. */
    void appendUpgrade(MinecraftPacketIds packet_id, PacketHandler handler);
    /** Adds to whatever downgrade handler is registered, or registers if none is. */
    void appendDowngrade(MinecraftPacketIds packet_id, PacketHandler handler);
    /** Replaces a registered upgrade handler. Throws if there is none. */
    void replaceUpgrade(MinecraftPacketIds packet_id, PacketHandler handler);
    /** Replaces a registered downgrade handler. Throws if there is none. */
    void replaceDowngrade(MinecraftPacketIds packet_id, PacketHandler handler);

    // --- transport axis: a base protocol ---

    /** Registers a handler for a packet travelling server to client. */
    void registerClientbound(MinecraftPacketIds packet_id, PacketHandler handler);
    /** Registers a handler for a packet travelling client to server. */
    void registerServerbound(MinecraftPacketIds packet_id, PacketHandler handler);
    /** Drops a packet travelling server to client. */
    void cancelClientbound(MinecraftPacketIds packet_id);
    /** Drops a packet travelling client to server. */
    void cancelServerbound(MinecraftPacketIds packet_id);

private:
    friend class ProtocolManager;

    void setPreviousVersion(int version);

    void registerAt(std::size_t slot, MinecraftPacketIds packet_id, PacketHandler handler);
    void appendAt(std::size_t slot, MinecraftPacketIds packet_id, PacketHandler handler);
    void replaceAt(std::size_t slot, MinecraftPacketIds packet_id, PacketHandler handler);

    // ViaVersion's PacketArrayMappings: indexed by packet id, so a hot-path lookup is one
    // bounds check and one index rather than a hash.
    std::array<std::vector<PacketHandler>, 2> mappings_;
    std::string name_;
    std::optional<int> version_;
    std::optional<int> previous_;
    bool initialized_ = false;
};

/**
 * A node in the version graph. Owns both directions of the wire diff between V and the node
 * registered before it, so a version step is described once, by the newer of its two ends.
 *
 * @note Declared and never defined, so registering a version with no protocols/vN/ directory
 * is a compile error rather than a silently handler-less vertex.
 */
template <ProtocolVersion V>
class Protocol;

} // namespace endweave
