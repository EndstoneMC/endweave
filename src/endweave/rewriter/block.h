#pragma once

#include "endweave/protocol/block.h"
#include "endweave/protocol/block_connection.h"
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
#include <bedrock/protocol/serializer.hpp>
#include <bedrock/protocol/sound.h>
#include <bedrock/protocol/stream.hpp>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <map>
#include <optional>
#include <set>
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

/** A sub-chunk and where it sits, counted in sub-chunks. */
struct PlacedSubChunk {
    bp::BlockPos pos;
    SubChunk sub_chunk;
};

/** Whether some hop from `From` to `To` gives blocks states BDS works out from their neighbours. */
template <int From, int To>
consteval bool connectsBlocks()
{
    if constexpr (From == To) {
        return false;
    }
    else {
        return HasConnections<From, step(From, To)> || connectsBlocks<step(From, To), To>();
    }
}

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
     * states the hop works out from neighbouring blocks, read from these sub-chunks or, for a
     * neighbour they do not hold, from the blocks the client was sent before. The client keeps these
     * ones too. */
    static void rewriteSubChunks(std::vector<PlacedSubChunk> &sub_chunks, BlockConnectionStorage &storage)
    {
        rewriteHops<From, To>(sub_chunks, storage);
        if constexpr (connectsBlocks<From, To>()) {
            for (const auto &placed : sub_chunks) {
                if (!placed.sub_chunk.storages.empty()) {
                    storage.store(placed.pos, placed.sub_chunk.storages.front());
                }
            }
        }
    }

    /** The id `To` gives the block `From` calls `id` at `pos`, with the states each hop works out
     * from the blocks the client was sent beside it. The client keeps it too. */
    static std::uint32_t rewriteBlock(const bp::BlockPos &pos, std::uint32_t id, BlockConnectionStorage &storage)
    {
        if constexpr (connectsBlocks<From, To>()) {
            const auto connected = connectBlock<From, To>(pos, id, storage);
            storage.set(pos, connected);
            return connected;
        }
        else {
            return getNewBlockStateId<From, To>(id);
        }
    }

    /** Works out again the blocks the client was sent around the `changed` ones, and keeps those
     * whose states change to send later, as the server sends nothing for them. Redstone reads the
     * wire a block up or down on each side, so this reaches past the faces.
     * @see ViaVersion ConnectionData#update. */
    static void updateNeighbors(std::span<const bp::BlockPos> changed, BlockConnectionStorage &storage)
    {
        if constexpr (connectsBlocks<From, To>()) {
            std::set<std::array<int, 3>> neighbors;
            for (const auto &pos : changed) {
                for (int x = -1; x <= 1; ++x) {
                    for (int y = -1; y <= 1; ++y) {
                        for (int z = -1; z <= 1; ++z) {
                            neighbors.insert({pos.x + x, pos.y + y, pos.z + z});
                        }
                    }
                }
            }
            for (const auto &pos : changed) {
                neighbors.erase({pos.x, pos.y, pos.z});
            }
            for (const auto &[x, y, z] : neighbors) {
                const bp::BlockPos pos{x, y, z};
                const auto sent = storage.get(pos);
                if (!sent) {
                    continue;
                }
                const auto connected = connectBlock<From, To>(pos, getNewBlockStateId<To, From>(*sent), storage);
                if (connected != *sent) {
                    storage.setUnsent(pos, connected);
                }
            }
        }
    }

    /** An UpdateBlockPacket for each block kept to send later. */
    static std::vector<std::string> writeUnsentBlocks(BlockConnectionStorage &storage)
    {
        std::vector<std::string> packets;
        for (const auto &[pos, id] : storage.takeUnsent()) {
            bp::packet_of_t<To, static_cast<int>(bp::MinecraftPacketIds::UpdateBlock)> packet;
            packet.pos = {pos.x, pos.y, pos.z};
            packet.runtime_id = bp::BlockRuntimeId{id};
            packet.update_flags = UPDATE_ALL;
            packet.layer = 0;
            bp::BinaryWriter out{packets.emplace_back()};
            bp::serialize(out, packet);
        }
        return packets;
    }

private:
    static constexpr std::uint8_t UPDATE_ALL = 3;

    /** The block `To` has at `pos` that the client was sent, as the hop to `B` calls it. */
    template <int B>
    static std::optional<std::uint32_t> getSent(const BlockConnectionStorage &storage, const bp::BlockPos &pos)
    {
        const auto id = storage.get(pos);
        return id ? std::optional{getNewBlockStateId<To, B>(*id)} : std::nullopt;
    }

    template <int A, int B>
    static void rewriteHops(std::vector<PlacedSubChunk> &sub_chunks, const BlockConnectionStorage &storage)
    {
        if constexpr (A != B) {
            constexpr int next = step(A, B);
            for (auto &placed : sub_chunks) {
                for (auto &blocks : placed.sub_chunk.storages) {
                    for (auto &id : blocks.getPalette()) {
                        id = getNewBlockStateId<A, next>(id);
                    }
                }
            }
            if constexpr (HasConnections<A, next>) {
                connectBlocks<A, next>(sub_chunks, storage);
            }
            rewriteHops<next, B>(sub_chunks, storage);
        }
    }

    template <int A, int B>
    static std::uint32_t connectBlock(const bp::BlockPos &pos, std::uint32_t id, const BlockConnectionStorage &storage)
    {
        if constexpr (A == B) {
            return id;
        }
        else {
            constexpr int next = step(A, B);
            auto mapped = getNewBlockStateId<A, next>(id);
            if constexpr (HasConnections<A, next>) {
                if (ConnectionData<A, next>::connects(mapped)) {
                    const auto get = [&](const bp::BlockPos &at) {
                        return getSent<next>(storage, at);
                    };
                    mapped = ConnectionData<A, next>::connect(pos, mapped, get).value_or(mapped);
                }
            }
            return connectBlock<next, B>(pos, mapped, storage);
        }
    }

    /** @see ViaVersion ConnectionData#connectBlocks. */
    template <int A, int B>
    static void connectBlocks(std::vector<PlacedSubChunk> &sub_chunks, const BlockConnectionStorage &storage)
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
                return getSent<B>(storage, pos);
            }
            return it->second->getElement(SubChunkStorage::getIndex(pos.x & 15, pos.y & 15, pos.z & 15));
        };

        std::vector<std::tuple<SubChunkStorage *, std::uint16_t, std::uint32_t>> changes;
        for (auto &placed : sub_chunks) {
            if (placed.sub_chunk.storages.empty()) {
                continue;
            }
            auto &layer = placed.sub_chunk.storages.front();
            std::vector<std::uint32_t> connecting;
            std::ranges::copy_if(layer.getPalette(), std::back_inserter(connecting), &ConnectionData<A, B>::connects);
            if (connecting.empty()) {
                continue;
            }
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    for (int y = 0; y < 16; ++y) {
                        const auto index = SubChunkStorage::getIndex(x, y, z);
                        const auto id = layer.getElement(index);
                        if (!id || std::ranges::find(connecting, *id) == connecting.end()) {
                            continue;
                        }
                        const bp::BlockPos pos{placed.pos.x * 16 + x, placed.pos.y * 16 + y, placed.pos.z * 16 + z};
                        const auto connected = ConnectionData<A, B>::connect(pos, *id, get);
                        if (connected && *connected != *id) {
                            changes.emplace_back(&layer, index, *connected);
                        }
                    }
                }
            }
        }
        for (const auto &[layer, index, id] : changes) {
            layer->setElement(index, id);
        }
    }
};

namespace detail {

/** Reads the `count` network sub-chunks at the front of `data`, in the column at `pos`, and returns
 * what follows them. A sub-chunk in format 9 says which one of the column it is. */
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
        const std::int32_t y = sub_chunk->version == 9 ? static_cast<std::int8_t>(sub_chunk->y_index)
                                                       : pos.y + static_cast<std::int32_t>(i);
        sub_chunks.push_back({bp::BlockPos{pos.x, y, pos.z}, std::move(*sub_chunk)});
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
    static std::expected<void, std::error_code> rewrite(bp::LevelChunkPacket_<From> &packet,
                                                        BlockConnectionStorage &storage)
    {
        // A request limit means the client asks for the sub-chunks, which then come in SubChunkPacket.
        if (packet.cache_enabled || packet.client_request_sub_chunk_limit.has_value()) {
            return {};
        }
        std::vector<PlacedSubChunk> sub_chunks;
        const auto rest = detail::readSubChunks(packet.serialized_chunk, packet.sub_chunks_count,
                                                {packet.pos.x, 0, packet.pos.z}, sub_chunks);
        if (!rest) {
            return std::unexpected(rest.error());
        }
        BlockRewriter<From, To>::rewriteSubChunks(sub_chunks, storage);
        packet.serialized_chunk = detail::writeSubChunks(sub_chunks, *rest);
        return {};
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::SubChunkPacket)> {
    static std::expected<void, std::error_code> rewrite(bp::SubChunkPacket_<From> &packet,
                                                        BlockConnectionStorage &storage)
    {
        std::vector<PlacedSubChunk> sub_chunks;
        std::vector<std::pair<std::string *, std::string>> entries;
        for (auto &entry : packet.sub_chunk_data) {
            if (!entry.serialized_sub_chunk || entry.serialized_sub_chunk->empty()) {
                continue;
            }
            const auto &center = packet.center_pos;
            const auto &offset = entry.sub_chunk_pos_offset;
            auto rest =
                detail::readSubChunks(*entry.serialized_sub_chunk, 1,
                                      {center.x + offset.x, center.y + offset.y, center.z + offset.z}, sub_chunks);
            if (!rest) {
                return std::unexpected(rest.error());
            }
            entries.emplace_back(&*entry.serialized_sub_chunk, std::move(*rest));
        }
        BlockRewriter<From, To>::rewriteSubChunks(sub_chunks, storage);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            *entries[i].first = detail::writeSubChunks(std::span{sub_chunks}.subspan(i, 1), entries[i].second);
        }
        return {};
    }
};

namespace detail {

/** An UpdateBlockPacket or UpdateBlockSyncedPacket: a block in layer 0 gets the states its neighbours
 * give it, and the client keeps it. */
template <int From, int To, class Packet>
void rewriteUpdateBlock(Packet &packet, BlockConnectionStorage &storage)
{
    if (packet.layer != 0) {
        packet.runtime_id = bp::BlockRuntimeId{getNewBlockStateId<From, To>(packet.runtime_id)};
        return;
    }
    const bp::BlockPos pos{packet.pos.x, packet.pos.y, packet.pos.z};
    packet.runtime_id = bp::BlockRuntimeId{BlockRewriter<From, To>::rewriteBlock(pos, packet.runtime_id, storage)};
    BlockRewriter<From, To>::updateNeighbors(std::span{&pos, 1}, storage);
}

} // namespace detail

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::UpdateBlock)> {
    static constexpr bool rewrites_block_ids = true;

    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::UpdateBlock)> &packet,
                        BlockConnectionStorage &storage)
    {
        detail::rewriteUpdateBlock<From, To>(packet, storage);
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::UpdateBlockSynced)> {
    static constexpr bool rewrites_block_ids = true;

    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::UpdateBlockSynced)> &packet,
                        BlockConnectionStorage &storage)
    {
        detail::rewriteUpdateBlock<From, To>(packet, storage);
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::UpdateSubChunkBlocks)> {
    static constexpr bool rewrites_block_ids = true;

    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::UpdateSubChunkBlocks)> &packet,
                        BlockConnectionStorage &storage)
    {
        // Every changed block first, so each one reads the others as they now are.
        for (const auto &block : packet.blocks_changed.standards) {
            storage.set({block.pos.x, block.pos.y, block.pos.z}, getNewBlockStateId<From, To>(block.runtime_id));
        }
        std::vector<bp::BlockPos> changed;
        for (auto &block : packet.blocks_changed.standards) {
            const bp::BlockPos pos{block.pos.x, block.pos.y, block.pos.z};
            block.runtime_id =
                bp::BlockRuntimeId{BlockRewriter<From, To>::rewriteBlock(pos, block.runtime_id, storage)};
            changed.push_back(pos);
        }
        BlockRewriter<From, To>::updateNeighbors(changed, storage);
        for (auto &block : packet.blocks_changed.extras) {
            block.runtime_id = bp::BlockRuntimeId{getNewBlockStateId<From, To>(block.runtime_id)};
        }
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::NetworkChunkPublisherUpdate)> {
    static void rewrite(
        bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::NetworkChunkPublisherUpdate)> &packet,
        BlockConnectionStorage &storage)
    {
        // A chunk to the side of one the client keeps can still be a neighbour.
        storage.unloadOutside({packet.position.x, packet.position.y, packet.position.z},
                              static_cast<std::int64_t>(packet.radius) + 32);
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::ChangeDimension)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::ChangeDimension)> &,
                        BlockConnectionStorage &storage)
    {
        storage.clear();
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
