#pragma once

#include "endweave/protocol/pipeline.h"

#include <any>
#include <chrono>
#include <endstone/logger.h>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace endweave {

class ProtocolManager;

/**
 * A connection's version pair and its pipeline, mirroring ViaVersion's ProtocolInfo.
 */
class ProtocolInfo {
public:
    /**
     * Constructs the info for a connection whose client version is not yet known.
     *
     * @param pipeline The connection's pipeline, seeded with the base protocols.
     * @param server_protocol_version The version the server itself speaks.
     */
    ProtocolInfo(ProtocolPipeline pipeline, int server_protocol_version)
        : pipeline_(std::move(pipeline)), server_protocol_version_(server_protocol_version)
    {
    }

    /**
     * Gets the protocol version the client speaks.
     *
     * @return The client's version, or 0 before the handshake has been seen.
     */
    [[nodiscard]] int getProtocolVersion() const
    {
        return protocol_version_;
    }

    /**
     * Records the protocol version the client speaks.
     *
     * @param version The client's version.
     */
    void setProtocolVersion(int version)
    {
        protocol_version_ = version;
    }

    /**
     * Gets the protocol version the server speaks.
     *
     * @return The server's version.
     */
    [[nodiscard]] int getServerProtocolVersion() const
    {
        return server_protocol_version_;
    }

    /**
     * Returns whether the client's version has been seen yet.
     *
     * @return true once the handshake has been handled.
     */
    [[nodiscard]] bool isKnown() const
    {
        return protocol_version_ != 0;
    }

    /**
     * Gets the connection's pipeline.
     *
     * @return The pipeline.
     */
    [[nodiscard]] ProtocolPipeline &getPipeline()
    {
        return pipeline_;
    }

private:
    ProtocolPipeline pipeline_;
    int protocol_version_ = 0;
    int server_protocol_version_ = 0;
};

/**
 * Per-connection translation state, mirroring ViaVersion's UserConnection.
 *
 * Carries the version pair and pipeline, a type-keyed store for stateful handlers, and the
 * registry itself -- ViaVersion reaches that through the Via global, endweave through the
 * connection a handler is already holding.
 *
 * @note Runs on the server thread. Nothing here is synchronised.
 */
class UserConnection {
public:
    /**
     * Constructs a connection with a base-only pipeline.
     *
     * @param protocol_manager The registry, which must outlive this connection.
     * @param logger The server logger.
     * @param address The peer address, which is what connections are keyed by.
     * @param server_protocol_version The version the server itself speaks.
     */
    UserConnection(ProtocolManager &protocol_manager, endstone::Logger &logger, std::string address,
                   int server_protocol_version);

    UserConnection(const UserConnection &) = delete;
    UserConnection &operator=(const UserConnection &) = delete;

    /**
     * Gets the protocol registry, for a handler that has to resolve a path.
     *
     * @return The registry.
     */
    [[nodiscard]] ProtocolManager &getProtocolManager() const
    {
        return *protocol_manager_;
    }

    /**
     * Gets the server logger.
     *
     * @return The logger.
     */
    [[nodiscard]] endstone::Logger &getLogger() const
    {
        return *logger_;
    }

    /**
     * Gets the peer address this connection is keyed by.
     *
     * @return The address.
     */
    [[nodiscard]] const std::string &getAddress() const
    {
        return address_;
    }

    /**
     * Gets the connection's version pair and pipeline.
     *
     * @return The protocol info.
     */
    [[nodiscard]] ProtocolInfo &getProtocolInfo()
    {
        return protocol_info_;
    }

    /**
     * Looks up a stored object by type, ViaVersion's StorableObject map.
     *
     * @return A pointer to the stored object, or nullptr if none is stored.
     */
    template <class T>
    [[nodiscard]] T *get()
    {
        const auto it = storage_.find(std::type_index(typeid(T)));
        return it == storage_.end() ? nullptr : std::any_cast<T>(&it->second);
    }

    /**
     * Stores an object by type, replacing any previous one.
     *
     * @param value The object to store.
     */
    template <class T>
    void put(T value)
    {
        storage_[std::type_index(typeid(T))] = std::move(value);
    }

    /**
     * Returns whether an object of the given type is stored.
     *
     * @return true if one is stored.
     */
    template <class T>
    [[nodiscard]] bool has() const
    {
        return storage_.contains(std::type_index(typeid(T)));
    }

    /**
     * Removes the stored object of the given type, if any.
     */
    template <class T>
    void remove()
    {
        storage_.erase(std::type_index(typeid(T)));
    }

    /**
     * Marks the connection as active, for the idle sweep.
     */
    void touch()
    {
        last_seen_ = std::chrono::steady_clock::now();
    }

    /**
     * Gets when the connection last carried a packet.
     *
     * @return The last-seen timestamp.
     */
    [[nodiscard]] std::chrono::steady_clock::time_point getLastSeen() const
    {
        return last_seen_;
    }

    /**
     * Reports a translation failure at most once per packet id.
     *
     * A malformed packet usually repeats, and one line per occurrence would drown the log.
     *
     * @param packet_id The packet id that failed.
     * @param error What the codec said.
     */
    void reportTranslationError(int packet_id, const std::error_code &error);

private:
    ProtocolManager *protocol_manager_;
    endstone::Logger *logger_;
    std::string address_;
    ProtocolInfo protocol_info_;
    std::unordered_map<std::type_index, std::any> storage_;
    std::unordered_map<int, std::error_code> reported_errors_;
    std::chrono::steady_clock::time_point last_seen_;
};

} // namespace endweave
