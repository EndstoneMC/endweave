#pragma once

#include "endweave/protocol/pipeline.h"

#include <any>
#include <chrono>
#include <endstone/endstone.hpp>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace endweave {

class ProtocolManager;

/**
 * A connection's version pair and its pipeline.
 *
 * @see ViaVersion ProtocolInfo (api) and ProtocolInfoImpl (common). State, username, uuid, and
 * compression are not tracked. endweave keys on the address and lets BDS own compression.
 */
class ProtocolInfo {
public:
    /**
     * @param pipeline The connection's pipeline, seeded with the base protocols.
     * @param server_protocol_version The version the server itself speaks.
     */
    ProtocolInfo(ProtocolPipeline pipeline, int server_protocol_version)
        : pipeline_(std::move(pipeline)), server_protocol_version_(server_protocol_version)
    {
    }

    /**
     * @return The client's version, or 0 before the handshake has been seen.
     * @see ViaVersion ProtocolInfo#protocolVersion.
     */
    [[nodiscard]] int getProtocolVersion() const
    {
        return protocol_version_;
    }

    /** @see ViaVersion ProtocolInfo#setProtocolVersion. */
    void setProtocolVersion(int version)
    {
        protocol_version_ = version;
    }

    /**
     * @return The version the server speaks.
     * @see ViaVersion ProtocolInfo#serverProtocolVersion.
     */
    [[nodiscard]] int getServerProtocolVersion() const
    {
        return server_protocol_version_;
    }

    /** @see ViaVersion ProtocolInfo#getPipeline. */
    [[nodiscard]] ProtocolPipeline &getPipeline()
    {
        return pipeline_;
    }

private:
    ProtocolPipeline pipeline_;       // ViaVersion: pipeline
    int protocol_version_ = 0;        // ViaVersion: protocolVersion
    int server_protocol_version_ = 0; // ViaVersion: serverProtocolVersion
};

/**
 * Per-connection translation state: the version pair, the pipeline, and a type-keyed store for
 * stateful handlers.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 *
 * @see ViaVersion UserConnection (api) and UserConnectionImpl (common). The netty channel,
 * packet tracker, entity trackers, and item hashers are dropped. The StorableObject map stays.
 */
class UserConnection {
public:
    /**
     * @param protocol_manager The registry, which must outlive this connection.
     * @param logger The server logger.
     * @param address The peer address, which is what connections are keyed by.
     * @param server_protocol_version The version the server itself speaks.
     * @see ViaVersion UserConnectionImpl(Channel, boolean).
     */
    UserConnection(ProtocolManager &protocol_manager, endstone::Logger &logger, endstone::SocketAddress address,
                   int server_protocol_version);

    UserConnection(const UserConnection &) = delete;
    UserConnection &operator=(const UserConnection &) = delete;

    /**
     * @return The protocol registry.
     * @note endweave-specific: ViaVersion uses the Via.getManager() global.
     */
    [[nodiscard]] ProtocolManager &getProtocolManager() const
    {
        return *protocol_manager_;
    }

    /**
     * @return The server logger.
     * @note endweave-specific: ViaVersion uses the Via.getPlatform().getLogger() global.
     */
    [[nodiscard]] endstone::Logger &getLogger() const
    {
        return *logger_;
    }

    /**
     * @return The peer address this connection is keyed by.
     * @note endweave-specific: ViaVersion keys by UUID once login completes.
     */
    [[nodiscard]] const endstone::SocketAddress &getAddress() const
    {
        return address_;
    }

    /** @see ViaVersion UserConnection#getProtocolInfo. */
    [[nodiscard]] ProtocolInfo &getProtocolInfo()
    {
        return protocol_info_;
    }

    /** @see ViaVersion UserConnection#get(Class). */
    template <class T>
    [[nodiscard]] T *get()
    {
        const auto it = storage_.find(std::type_index(typeid(T)));
        return it == storage_.end() ? nullptr : std::any_cast<T>(&it->second);
    }

    /** @see ViaVersion UserConnection#put(StorableObject). */
    template <class T>
    void put(T value)
    {
        storage_[std::type_index(typeid(T))] = std::move(value);
    }

    /** @see ViaVersion UserConnection#has(Class). */
    template <class T>
    [[nodiscard]] bool has() const
    {
        return storage_.contains(std::type_index(typeid(T)));
    }

    /** @see ViaVersion UserConnection#remove(Class). */
    template <class T>
    void remove()
    {
        storage_.erase(std::type_index(typeid(T)));
    }

    /**
     * Marks the connection as active, for the idle sweep.
     *
     * @note endweave-specific: ViaVersion cleans up on the netty channel-close future.
     */
    void touch()
    {
        last_seen_ = std::chrono::steady_clock::now();
    }

    /** @return When the connection last carried a packet. */
    [[nodiscard]] std::chrono::steady_clock::time_point getLastSeen() const
    {
        return last_seen_;
    }

    /**
     * Reports a translation failure at most once per packet id.
     *
     * @param packet_id The packet id that failed.
     * @param error Why the packet did not make it through.
     * @see ViaVersion AbstractProtocol#printRemapError (endweave de-duplicates per packet id).
     */
    void reportTranslationError(int packet_id, PacketError error);

private:
    ProtocolManager *protocol_manager_; // endweave-specific
    endstone::Logger *logger_;          // endweave-specific
    endstone::SocketAddress address_;   // endweave-specific key
    ProtocolInfo protocol_info_;        // ViaVersion: protocolInfo
    std::unordered_map<std::type_index, std::any> storage_; // ViaVersion: storedObjects
    std::unordered_map<int, PacketError> reported_errors_; // endweave-specific: reportTranslationError de-dup
    std::chrono::steady_clock::time_point last_seen_;       // endweave-specific: idle sweep
};

} // namespace endweave
