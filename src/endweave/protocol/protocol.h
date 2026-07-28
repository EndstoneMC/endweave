#pragma once

#include "endweave/protocol/direction.h"
#include "endweave/protocol/handler.h"
#include "endweave/protocol/packet.h"
#include "endweave/protocol/version.h"

#include <array>
#include <bedrock/protocol.hpp>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace endweave {

class ProtocolManager;
class UserConnection;

/**
 * Two packet-id-keyed handler tables plus the transform that runs a packet body through them.
 *
 * A base protocol addresses the tables as a transport Direction, a version node as a Step.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 *
 * @see ViaVersion Protocol (api) and AbstractProtocol (common), collapsed into one class.
 */
class AbstractProtocol {
public:
    virtual ~AbstractProtocol() = default;

    AbstractProtocol(const AbstractProtocol &) = delete;
    AbstractProtocol &operator=(const AbstractProtocol &) = delete;

    /**
     * Runs registerPackets() exactly once. Throws std::logic_error if called again.
     *
     * @see ViaVersion AbstractProtocol#initialize.
     */
    void initialize();

    /**
     * Sets up per-connection state. May be called more than once for one connection.
     *
     * @see ViaVersion Protocol#init.
     */
    virtual void init(UserConnection &connection)
    {
        (void)connection;
    }

    /**
     * @return The version this node speaks, or std::nullopt for a base protocol.
     * @see ViaVersion AbstractProtocol#getClientVersion (the higher end of the edge).
     */
    [[nodiscard]] std::optional<int> getVersion() const
    {
        return version_;
    }

    /**
     * @return The predecessor's version, or std::nullopt for the chain root and base protocols.
     * @see ViaVersion AbstractProtocol#getServerVersion (the lower end of the edge).
     */
    [[nodiscard]] std::optional<int> getPreviousVersion() const
    {
        return previous_;
    }

    /**
     * @return true if this protocol stays at the head of every pipeline and does no translation.
     * @see ViaVersion Protocol#isBaseProtocol.
     */
    [[nodiscard]] virtual bool isBaseProtocol() const
    {
        return false;
    }

    /**
     * @return The version number for a node ("1001"), or "base".
     * @see ViaVersion AbstractProtocol#toString (C++ has no getClass().getSimpleName()).
     */
    [[nodiscard]] const std::string &getName() const
    {
        return name_;
    }

    /**
     * Checks whether a packet id has a handler in the given table.
     *
     * @param slot The table, from slotOf(Direction) or slotOf(Step).
     * @param packet_id The packet id.
     * @return true if a handler is registered.
     * @see ViaVersion Protocol#hasRegisteredClientbound / #hasRegisteredServerbound.
     */
    [[nodiscard]] bool hasMapping(std::size_t slot, int packet_id) const;

    /**
     * Runs a packet through the handler for the given id. The caller checks hasMapping() first,
     * so an unmapped id must not reach here.
     *
     * @param slot The table, from slotOf(Direction) or slotOf(Step).
     * @param packet_id The packet id.
     * @param connection The connection the packet belongs to.
     * @param packet The packet in flight, which the handler decodes and replaces.
     * @return Nothing once translated, PacketError::Cancelled if the handler dropped the packet,
     * or why it could not be translated.
     * @see ViaVersion AbstractProtocol#transform.
     */
    [[nodiscard]] std::expected<void, PacketError> transform(std::size_t slot, int packet_id,
                                                             UserConnection &connection, PacketHolder &packet) const;

protected:
    /** Constructs a base protocol. */
    explicit AbstractProtocol(std::string name);

    /** Constructs a version node, named for its version. */
    explicit AbstractProtocol(ProtocolVersion version);

    /**
     * Registers the packet handlers for this protocol. To be overridden.
     *
     * @see ViaVersion AbstractProtocol#registerPackets.
     */
    virtual void registerPackets() {}

    // Version-step axis: a Protocol<V> node. Keyed by Upgrade/Downgrade because the same edge is
    // walked in both transport directions across connections. A converter names the two shapes it
    // maps between, and the id is the one they share.

    /** @see ViaVersion AbstractProtocol#registerServerbound. */
    template <class In, class Out>
    void registerUpgrade(PacketConverter<In, Out> converter)
    {
        registerAt(slotOf(Step::Upgrade), In::Id, makePacketHandler(converter));
    }

    /** @see ViaBackwards BackwardsProtocol#registerClientbound. */
    template <class In, class Out>
    void registerDowngrade(PacketConverter<In, Out> converter)
    {
        registerAt(slotOf(Step::Downgrade), In::Id, makePacketHandler(converter));
    }

    /** @see ViaVersion AbstractProtocol#cancelServerbound. */
    void cancelUpgrade(bedrock::protocol::MinecraftPacketIds packet_id);
    /** @see ViaBackwards BackwardsProtocol#cancelClientbound. */
    void cancelDowngrade(bedrock::protocol::MinecraftPacketIds packet_id);

    /** @see ViaVersion AbstractProtocol#appendServerbound. */
    template <class In, class Out>
    void appendUpgrade(PacketConverter<In, Out> converter)
    {
        appendAt(slotOf(Step::Upgrade), In::Id, makePacketHandler(converter));
    }

    /** @see ViaVersion AbstractProtocol#appendClientbound. */
    template <class In, class Out>
    void appendDowngrade(PacketConverter<In, Out> converter)
    {
        appendAt(slotOf(Step::Downgrade), In::Id, makePacketHandler(converter));
    }

    /** @see ViaVersion AbstractProtocol#replaceServerbound. */
    template <class In, class Out>
    void replaceUpgrade(PacketConverter<In, Out> converter)
    {
        replaceAt(slotOf(Step::Upgrade), In::Id, makePacketHandler(converter));
    }

    /** @see ViaVersion AbstractProtocol#replaceClientbound. */
    template <class In, class Out>
    void replaceDowngrade(PacketConverter<In, Out> converter)
    {
        replaceAt(slotOf(Step::Downgrade), In::Id, makePacketHandler(converter));
    }

    // Transport axis: a base protocol.

    /** @see ViaVersion AbstractProtocol#registerClientbound. */
    template <class In, class Out>
    void registerClientbound(PacketConverter<In, Out> converter)
    {
        registerAt(slotOf(Direction::Clientbound), In::Id, makePacketHandler(converter));
    }

    /** @see ViaVersion AbstractProtocol#registerServerbound. */
    template <class In, class Out>
    void registerServerbound(PacketConverter<In, Out> converter)
    {
        registerAt(slotOf(Direction::Serverbound), In::Id, makePacketHandler(converter));
    }

    /** @see ViaVersion AbstractProtocol#cancelClientbound. */
    void cancelClientbound(bedrock::protocol::MinecraftPacketIds packet_id);
    /** @see ViaVersion AbstractProtocol#cancelServerbound. */
    void cancelServerbound(bedrock::protocol::MinecraftPacketIds packet_id);

private:
    friend class ProtocolManager;

    void setPreviousVersion(int version);

    void registerAt(std::size_t slot, int packet_id, PacketHandler handler);
    void appendAt(std::size_t slot, int packet_id, PacketHandler handler);
    void replaceAt(std::size_t slot, int packet_id, PacketHandler handler);

    std::array<std::vector<PacketHandler>, 2> mappings_; // ViaVersion: clientboundMappings + serverboundMappings
    std::string name_;              // ViaVersion: getClass().getSimpleName()
    std::optional<int> version_;    // ViaVersion: clientVersion
    std::optional<int> previous_;   // ViaVersion: serverVersion
    bool initialized_ = false;      // ViaVersion: initialized
};

/**
 * A node in the version graph, owning both directions of the wire diff between V and the node
 * registered before it. Declared and never defined, so registering a version with no
 * protocols/vN/ directory is a compile error.
 *
 * @see A ViaVersion forward protocol (e.g. Protocol1_20To1_20_2) fused with its ViaBackwards
 * backward protocol (Protocol1_20_2To1_20).
 */
template <ProtocolVersion V>
class Protocol;

} // namespace endweave
