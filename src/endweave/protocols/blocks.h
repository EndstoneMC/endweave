#pragma once

#include <array>
#include <bedrock/protocol.hpp>
#include <bedrock/protocol/detail/reflect.hpp>
#include <cstdint>
#include <expected>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

namespace endweave::blocks {

using Lookup = std::function<std::uint32_t(int, int, int, int)>;
struct WorldContext {
    Lookup lookup;
    int dimension = 0;
    std::map<std::array<int, 4>, std::uint32_t> cache;
    std::set<std::array<int, 3>> changed;
};

std::uint32_t mapId(std::uint32_t id, bool to_new);
std::uint32_t connectedId(std::uint32_t id, int x, int y, int z, WorldContext &world);
std::vector<std::string> neighborUpdates(WorldContext &world);
std::expected<void, std::error_code> rewriteSubChunks(std::string &data, std::uint32_t count, bool to_new,
                                                      WorldContext *world = nullptr, int x = 0, int y = 0, int z = 0);

template <class T>
struct Children {
    static constexpr bool value = false;
};
template <class T>
consteval bool containsIds();
template <class T>
struct Children<std::optional<T>> {
    static constexpr bool value = containsIds<T>();
};
template <class T>
struct Children<std::vector<T>> {
    static constexpr bool value = containsIds<T>();
};
template <class T, std::size_t N>
struct Children<std::array<T, N>> {
    static constexpr bool value = containsIds<T>();
};
template <class K, class V>
struct Children<std::map<K, V>> {
    static constexpr bool value = containsIds<V>();
};
template <class... T>
struct Children<std::variant<T...>> {
    static constexpr bool value = (containsIds<T>() || ...);
};

template <class T, std::size_t I>
consteval bool isId()
{
    constexpr auto name = bedrock::protocol::field_name<I, T>();
    if constexpr (name == "block_runtime_id" || name == "target_block_id") {
        return true;
    }
    constexpr auto type = bedrock::protocol::struct_name<T>();
    return name == "runtime_id" && (type == "UpdateBlockPacket" || type == "UpdateBlockSyncedPacket" ||
                                    type == "UpdateSubChunkNetworkBlockInfo");
}

template <class T>
consteval bool containsIds()
{
    namespace bp = bedrock::protocol;
    if constexpr (bp::Reflected<T>) {
        return []<std::size_t... I>(std::index_sequence<I...>) {
            return (
                (isId<T, I>() || containsIds<std::remove_cvref_t<decltype(bp::field_get<I>(std::declval<T &>()))>>()) ||
                ...);
        }(std::make_index_sequence<bp::field_count<T>()>{});
    }
    return Children<T>::value;
}

template <class T>
void rewriteIds(T &value, bool to_new, WorldContext *world = nullptr)
{
    namespace bp = bedrock::protocol;
    if constexpr (bp::Reflected<T>) {
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (
                [&] {
                    auto &field = bp::field_get<I>(value);
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (isId<T, I>()) {
                        auto id = static_cast<std::uint32_t>(field);
                        if constexpr (bp::field_name<I, T>() == "runtime_id" && requires { value.pos; }) {
                            if (world && to_new) {
                                world->changed.insert({value.pos.x, value.pos.y, value.pos.z});
                                field = static_cast<F>(connectedId(id, value.pos.x, value.pos.y, value.pos.z, *world));
                            }
                            else {
                                field = static_cast<F>(mapId(id, to_new));
                            }
                        }
                        else {
                            field = static_cast<F>(mapId(id, to_new));
                        }
                    }
                    else if constexpr (containsIds<F>()) {
                        rewriteIds(field, to_new, world);
                    }
                }(),
                ...);
        }(std::make_index_sequence<bp::field_count<T>()>{});
    }
    else if constexpr (requires {
                           std::visit([](auto &) {}, value);
                           value.index();
                       }) {
        std::visit(
            [&](auto &child) {
                rewriteIds(child, to_new, world);
            },
            value);
    }
    else if constexpr (requires { value.has_value(); }) {
        if (value) {
            rewriteIds(*value, to_new, world);
        }
    }
    else if constexpr (requires {
                           value.begin();
                           value.end();
                       }) {
        for (auto &child : value) {
            if constexpr (requires { child.second; }) {
                rewriteIds(child.second, to_new, world);
            }
            else {
                rewriteIds(child, to_new, world);
            }
        }
    }
}

template <int From, int To, int Id>
inline constexpr bool needs_rewrite =
    (From < 2193) != (To < 2193) &&
    (containsIds<bedrock::protocol::packet_of_t<From, Id>>() || Id == 58 || Id == 174 || Id == 129);

template <int From, int To, int Id, class Packet>
std::expected<void, std::error_code> rewrite(Packet &packet, WorldContext *world)
{
    constexpr bool to_new = To >= 2193;
    if constexpr (containsIds<Packet>()) {
        rewriteIds(packet, to_new, world);
    }
    if constexpr (Id == 129) {
        // Palette bytes change, so the server's cached blob hashes no longer describe them.
        // Negotiate uncached chunks before the server streams any world data.
        packet.enabled = false;
    }
    if constexpr (Id == 58) {
        if (world) {
            world->dimension = static_cast<int>(packet.dimension_id);
        }
        if (packet.sub_chunks_count < 0xfffffffeu) {
            return rewriteSubChunks(packet.serialized_chunk, packet.sub_chunks_count, to_new, world, packet.pos.x * 16,
                                    static_cast<int>(packet.dimension_id) == 0 ? -64 : 0, packet.pos.z * 16);
        }
    }
    if constexpr (Id == 174) {
        if (world) {
            world->dimension = static_cast<int>(packet.dimension_type);
        }
        for (auto &entry : packet.sub_chunk_data) {
            if (entry.serialized_sub_chunk && !entry.serialized_sub_chunk->empty()) {
                auto result = rewriteSubChunks(*entry.serialized_sub_chunk, 1, to_new, world,
                                               (packet.center_pos.x + entry.sub_chunk_pos_offset.x) * 16,
                                               (packet.center_pos.y + entry.sub_chunk_pos_offset.y) * 16,
                                               (packet.center_pos.z + entry.sub_chunk_pos_offset.z) * 16);
                if (!result) {
                    return result;
                }
            }
        }
    }
    return {};
}

} // namespace endweave::blocks
