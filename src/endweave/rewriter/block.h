#pragma once

#include "endweave/protocol/block.h"
#include "endweave/protocol/rewrite.h"
#include "endweave/protocol/sub_chunk.h"

#include <algorithm>
#include <array>
#include <bedrock/protocol/actor.h>
#include <bedrock/protocol/block.h>
#include <bedrock/protocol/chunk.h>
#include <bedrock/protocol/common.h>
#include <bedrock/protocol/detail/reflect.hpp>
#include <bedrock/protocol/network.h>
#include <bedrock/protocol/sound.h>
#include <bedrock/protocol/stream.hpp>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace bp = bedrock::protocol;

namespace endweave {

namespace detail {

template <class T>
consteval bool carriesBlockIds();

// BDS types this one member int32_t, not BlockRuntimeId.
template <class T>
consteval bool isItemInstanceDescriptor()
{
    if constexpr (bp::Reflected<T>) {
        return bp::struct_name<T>() == "SerializedNetworkItemInstanceDescriptor" &&
               bp::field_index<T>("block_runtime_id") != bp::field_npos;
    }
    else {
        return false;
    }
}

template <class T>
struct CarriesBlockIds {
    static consteval bool get()
    {
        if constexpr (std::is_same_v<T, bp::BlockRuntimeId> || isItemInstanceDescriptor<T>()) {
            return true;
        }
        else if constexpr (bp::Reflected<T>) {
            return []<std::size_t... I>(std::index_sequence<I...>) {
                return (carriesBlockIds<std::remove_cvref_t<decltype(bp::field_get<I>(std::declval<T &>()))>>() || ...);
            }(std::make_index_sequence<bp::field_count<T>()>{});
        }
        else {
            return false;
        }
    }
};

template <class T>
struct CarriesBlockIds<std::optional<T>> {
    static consteval bool get()
    {
        return carriesBlockIds<T>();
    }
};

template <class T, class A>
struct CarriesBlockIds<std::vector<T, A>> {
    static consteval bool get()
    {
        return carriesBlockIds<T>();
    }
};

template <class T, std::size_t N>
struct CarriesBlockIds<std::array<T, N>> {
    static consteval bool get()
    {
        return carriesBlockIds<T>();
    }
};

template <class K, class V, class C, class A>
struct CarriesBlockIds<std::map<K, V, C, A>> {
    static consteval bool get()
    {
        return carriesBlockIds<V>();
    }
};

template <class... T>
struct CarriesBlockIds<std::variant<T...>> {
    static consteval bool get()
    {
        return (carriesBlockIds<T>() || ...);
    }
};

template <class T>
consteval bool carriesBlockIds()
{
    return CarriesBlockIds<T>::get();
}

} // namespace detail

/** Whether a value holds a block id anywhere inside it. */
template <class T>
inline constexpr bool carries_block_ids_v = detail::carriesBlockIds<T>();

/** A sub-chunk where it sits among the ones its packet carries, counted in sub-chunks. */
struct PlacedSubChunk {
    bp::BlockPos pos;
    SubChunk sub_chunk;
};

/** Gives the blocks a packet carries the ids `To` numbers them by.
 * @see ViaVersion BlockRewriter. */
template <int From, int To>
struct BlockRewriter {
    static std::int32_t rewrite(std::int32_t id)
    {
        return static_cast<std::int32_t>(getNewBlockStateId<From, To>(static_cast<std::uint32_t>(id)));
    }

    /** Every `BlockRuntimeId` inside `value`. */
    template <class T>
    static void rewriteBlockIds(T &value)
    {
        if constexpr (!carries_block_ids_v<T>) {
            return;
        }
        else if constexpr (std::is_same_v<T, bp::BlockRuntimeId>) {
            value = bp::BlockRuntimeId{getNewBlockStateId<From, To>(value)};
        }
        else if constexpr (detail::isItemInstanceDescriptor<T>()) {
            value.block_runtime_id = rewrite(value.block_runtime_id);
        }
        else if constexpr (bp::Reflected<T>) {
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                (rewriteBlockIds(bp::field_get<I>(value)), ...);
            }(std::make_index_sequence<bp::field_count<T>()>{});
        }
        else if constexpr (requires { value.index(); }) {
            std::visit(
                [](auto &alternative) {
                    rewriteBlockIds(alternative);
                },
                value);
        }
        else if constexpr (requires { value.has_value(); }) {
            if (value.has_value()) {
                rewriteBlockIds(*value);
            }
        }
        else if constexpr (requires { typename T::mapped_type; }) {
            for (auto &entry : value) {
                rewriteBlockIds(entry.second);
            }
        }
        else {
            for (auto &element : value) {
                rewriteBlockIds(element);
            }
        }
    }

    /** The block ids actor data holds: a minecart's display block, an enderman's carried block, and
     * a falling block's variant. */
    static void rewriteActorData(std::vector<bp::DataItemEntry_<From>> &entries, bool falling_block)
    {
        using Ids = bp::ActorDataIDs_<From>;
        for (auto &entry : entries) {
            auto *const value = std::get_if<bp::DataItemIntPayload_<From>>(&entry.payload);
            if (value != nullptr && (entry.id == static_cast<std::uint32_t>(Ids::DisplayTileRuntimeId) ||
                                     entry.id == static_cast<std::uint32_t>(Ids::DeprecatedCarryBlockRuntimeId) ||
                                     (falling_block && entry.id == static_cast<std::uint32_t>(Ids::Variant)))) {
                value->value = rewrite(value->value);
            }
        }
    }

    /** The blocks of the sub-chunks one packet carries, a hop at a time: each hop's ids, then the
     * states the hop works out from neighbouring blocks, read from these sub-chunks alone. */
    static void rewriteSubChunks(std::vector<PlacedSubChunk> &sub_chunks)
    {
        rewriteHops<From, To>(sub_chunks);
    }

private:
    template <int A, int B>
    static void rewriteHops(std::vector<PlacedSubChunk> &sub_chunks)
    {
        if constexpr (A != B) {
            constexpr int next = step(A, B);
            for (auto &placed : sub_chunks) {
                for (auto &storage : placed.sub_chunk.storages) {
                    for (auto &id : storage.getPalette()) {
                        id = getNewBlockStateId<A, next>(id);
                    }
                }
            }
            if constexpr (HasConnections<A, next>) {
                connectBlocks<A, next>(sub_chunks);
            }
            rewriteHops<next, B>(sub_chunks);
        }
    }

    /** @see ViaVersion ConnectionData#connectBlocks. */
    template <int A, int B>
    static void connectBlocks(std::vector<PlacedSubChunk> &sub_chunks)
    {
        std::map<std::tuple<int, int, int>, const SubChunkStorage *> blocks;
        for (const auto &placed : sub_chunks) {
            if (!placed.sub_chunk.storages.empty()) {
                blocks.emplace(std::tuple{placed.pos.x, placed.pos.y, placed.pos.z},
                               &placed.sub_chunk.storages.front());
            }
        }
        const auto get = [&](const bp::BlockPos &pos) -> std::optional<std::uint32_t> {
            const auto it = blocks.find({pos.x >> 4, pos.y >> 4, pos.z >> 4});
            if (it == blocks.end()) {
                return std::nullopt;
            }
            return it->second->getElement(SubChunkStorage::getIndex(pos.x & 15, pos.y & 15, pos.z & 15));
        };

        std::vector<std::tuple<SubChunkStorage *, std::uint16_t, std::uint32_t>> changes;
        for (auto &placed : sub_chunks) {
            if (placed.sub_chunk.storages.empty()) {
                continue;
            }
            auto &storage = placed.sub_chunk.storages.front();
            std::vector<std::uint32_t> connecting;
            std::ranges::copy_if(storage.getPalette(), std::back_inserter(connecting), &ConnectionData<A, B>::connects);
            if (connecting.empty()) {
                continue;
            }
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    for (int y = 0; y < 16; ++y) {
                        const auto index = SubChunkStorage::getIndex(x, y, z);
                        const auto id = storage.getElement(index);
                        if (!id || std::ranges::find(connecting, *id) == connecting.end()) {
                            continue;
                        }
                        const bp::BlockPos pos{placed.pos.x * 16 + x, placed.pos.y * 16 + y, placed.pos.z * 16 + z};
                        const auto connected = ConnectionData<A, B>::connect(pos, *id, get);
                        if (connected && *connected != *id) {
                            changes.emplace_back(&storage, index, *connected);
                        }
                    }
                }
            }
        }
        for (const auto &[storage, index, id] : changes) {
            storage->setElement(index, id);
        }
    }
};

namespace detail {

/** Reads the `count` network sub-chunks at the front of `data`, placed one above the other from
 * `pos`, and returns what follows them. */
inline std::expected<std::string, std::error_code> readSubChunks(const std::string &data, std::size_t count,
                                                                 bp::BlockPos pos,
                                                                 std::vector<PlacedSubChunk> &sub_chunks)
{
    bp::BinaryReader in{data};
    for (std::size_t i = 0; i < count; ++i) {
        auto sub_chunk = SubChunk::read(in);
        if (!sub_chunk) {
            return std::unexpected(sub_chunk.error());
        }
        sub_chunks.push_back({bp::BlockPos{pos.x, pos.y + static_cast<std::int32_t>(i), pos.z}, std::move(*sub_chunk)});
    }
    return std::string(in.getView().substr(in.getReadPointer()));
}

inline std::string writeSubChunks(std::span<const PlacedSubChunk> sub_chunks, std::string_view rest)
{
    std::string data;
    bp::BinaryWriter out{data};
    for (const auto &placed : sub_chunks) {
        placed.sub_chunk.write(out);
    }
    out.writeRawBytes(rest);
    return data;
}

} // namespace detail

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::LevelEvent)> {
    // BDS LevelEvent values whose data is a block.
    enum class BlockEvent : std::int32_t {
        ParticlesDestroyBlock = 2001,
        ParticlesDestroyBlockNoSound = 2021,
        ParticlesCrackBlockDown = 3603,
        ParticlesCrackBlockUp = 3604,
        ParticlesCrackBlockNorth = 3605,
        ParticlesCrackBlockSouth = 3606,
        ParticlesCrackBlockWest = 3607,
        ParticlesCrackBlockEast = 3608,
    };

    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::LevelEvent)> &packet)
    {
        switch (static_cast<BlockEvent>(packet.event_id)) {
        case BlockEvent::ParticlesDestroyBlock:
        case BlockEvent::ParticlesDestroyBlockNoSound:
        case BlockEvent::ParticlesCrackBlockDown:
        case BlockEvent::ParticlesCrackBlockUp:
        case BlockEvent::ParticlesCrackBlockNorth:
        case BlockEvent::ParticlesCrackBlockSouth:
        case BlockEvent::ParticlesCrackBlockWest:
        case BlockEvent::ParticlesCrackBlockEast:
            packet.data = BlockRewriter<From, To>::rewrite(packet.data);
            break;
        default:
            break;
        }
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::LevelSoundEvent)> {
    static void rewrite(bp::LevelSoundEventPacket_<From> &packet)
    {
        // The sound events whose data is the block they sound for.
        for (const std::string_view event : {"item.use.on", "hit", "step", "break", "place", "land"}) {
            if (packet.sound_event == event) {
                packet.data = BlockRewriter<From, To>::rewrite(packet.data);
                return;
            }
        }
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::FullChunkData)> {
    static std::expected<void, std::error_code> rewrite(bp::LevelChunkPacket_<From> &packet)
    {
        // A request limit means the client asks for the sub-chunks, which then come in SubChunkPacket.
        if (packet.cache_enabled || packet.client_request_sub_chunk_limit.has_value()) {
            return {};
        }
        std::vector<PlacedSubChunk> sub_chunks;
        const auto rest = detail::readSubChunks(packet.serialized_chunk, packet.sub_chunks_count, {}, sub_chunks);
        if (!rest) {
            return std::unexpected(rest.error());
        }
        BlockRewriter<From, To>::rewriteSubChunks(sub_chunks);
        packet.serialized_chunk = detail::writeSubChunks(sub_chunks, *rest);
        return {};
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::SubChunkPacket)> {
    static std::expected<void, std::error_code> rewrite(bp::SubChunkPacket_<From> &packet)
    {
        std::vector<PlacedSubChunk> sub_chunks;
        std::vector<std::pair<std::string *, std::string>> entries;
        for (auto &entry : packet.sub_chunk_data) {
            if (!entry.serialized_sub_chunk || entry.serialized_sub_chunk->empty()) {
                continue;
            }
            const auto &offset = entry.sub_chunk_pos_offset;
            auto rest =
                detail::readSubChunks(*entry.serialized_sub_chunk, 1, {offset.x, offset.y, offset.z}, sub_chunks);
            if (!rest) {
                return std::unexpected(rest.error());
            }
            entries.emplace_back(&*entry.serialized_sub_chunk, std::move(*rest));
        }
        BlockRewriter<From, To>::rewriteSubChunks(sub_chunks);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            *entries[i].first = detail::writeSubChunks(std::span{sub_chunks}.subspan(i, 1), entries[i].second);
        }
        return {};
    }
};

// ENDWEAVE: the server's blob ids hash the sub-chunks it serializes, so they stop naming what the
// client is sent once its palettes are rewritten. The client is made to ask for chunks uncached.
template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::ClientCacheStatus)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::ClientCacheStatus)> &packet)
    {
        packet.enabled = false;
    }
};

} // namespace endweave
