#pragma once

#include "endweave/protocol/handler.h"

#include <any>
#include <cstddef>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace endweave {

/** What a packet costs on a connection, before any buffer is built. Queried by id from a table the
 * caller holds, so a packet that needs nothing never crosses back into the engine. */
enum class Action : char {
    Passthrough = 0,
    Translate = 1,
    Cancel = 2,
};

/** `PacketHeader` packs the id in ten bits, so this covers every id the wire can carry and an
 * index into a table of this size can never be out of range. */
inline constexpr std::size_t kActionTableSize = 1024;

/** One connection's two directions, resolved once. Everything after is an indexed load.
 * @see ViaVersion UserConnection (api) and UserConnectionImpl (common). */
class Session {
public:
    Session(ProtocolVersion client_version, ProtocolVersion server_version)
        : client_version_(client_version), server_version_(server_version),
          serverbound_(getPacketHandlers(client_version, server_version)),
          clientbound_(getPacketHandlers(server_version, client_version))
    {
    }

    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;

    /** @see ViaVersion UserConnection#getProtocolInfo. */
    [[nodiscard]] ProtocolVersion getClientVersion() const
    {
        return client_version_;
    }

    [[nodiscard]] ProtocolVersion getServerVersion() const
    {
        return server_version_;
    }

    [[nodiscard]] const PacketHandlers &getServerboundHandlers() const
    {
        return serverbound_;
    }

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

private:
    ProtocolVersion client_version_;
    ProtocolVersion server_version_;
    PacketHandlers serverbound_;
    PacketHandlers clientbound_;
    std::unordered_map<std::type_index, std::any> storage_;
};

/** The per-id cost of one direction, laid out for the caller to index. Cancelled is asked first,
 * which is the order the engine itself uses: a packet the destination cannot have builds no
 * buffer either way. */
inline std::string actionTable(const PacketHandlers &handlers)
{
    std::string table(kActionTableSize, static_cast<char>(Action::Passthrough));
    for (std::size_t id = 0; id < kActionTableSize; ++id) {
        const int packet_id = static_cast<int>(id);
        if (handlers.isCancelled(packet_id)) {
            table[id] = static_cast<char>(Action::Cancel);
        }
        else if (handlers.get(packet_id) != nullptr) {
            table[id] = static_cast<char>(Action::Translate);
        }
    }
    return table;
}

} // namespace endweave
