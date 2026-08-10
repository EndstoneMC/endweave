#pragma once

#include "endweave/protocol/handler.h"

#include <any>
#include <chrono>
#include <endstone/endstone.hpp>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace endweave {

/** @see ViaVersion UserConnection (api) and UserConnectionImpl (common). */
class UserConnection {
public:
    explicit UserConnection(endstone::SocketAddress address)
        : address_(std::move(address)), last_seen_(std::chrono::steady_clock::now())
    {
    }

    UserConnection(const UserConnection &) = delete;
    UserConnection &operator=(const UserConnection &) = delete;

    [[nodiscard]] const endstone::SocketAddress &getAddress() const
    {
        return address_;
    }

    /** @see ViaVersion UserConnection#getProtocolInfo. */
    [[nodiscard]] ProtocolVersion getClientVersion() const
    {
        return client_version_;
    }

    /** Resolves both handler tables once, when the client announces itself. Everything
     * after that is an indexed load. */
    void setClientVersion(ProtocolVersion version, ProtocolVersion server_version)
    {
        client_version_ = version;
        serverbound_ = getPacketHandlers(version, server_version);
        clientbound_ = getPacketHandlers(server_version, version);
    }

    /** @see ViaVersion Protocol#cancelServerbound. */
    [[nodiscard]] const PacketHandlers &getServerboundHandlers() const
    {
        return serverbound_;
    }

    /** @see ViaVersion Protocol#cancelClientbound. */
    [[nodiscard]] const PacketHandlers &getClientboundHandlers() const
    {
        return clientbound_;
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

    void touch()
    {
        last_seen_ = std::chrono::steady_clock::now();
    }

    [[nodiscard]] std::chrono::steady_clock::time_point getLastSeen() const
    {
        return last_seen_;
    }

private:
    endstone::SocketAddress address_;
    ProtocolVersion client_version_ = ProtocolVersion::UNKNOWN;
    PacketHandlers serverbound_;
    PacketHandlers clientbound_;
    std::unordered_map<std::type_index, std::any> storage_;
    std::chrono::steady_clock::time_point last_seen_;
};

} // namespace endweave
