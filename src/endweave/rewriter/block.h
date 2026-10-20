#pragma once

#include "endweave/protocol/block_connection.h"
#include "endweave/protocol/chunk.h"
#include "endweave/protocol/mapping.h"
#include "endweave/protocol/rewrite.h"

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
#include <map>
#include <optional>
#include <ranges>
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
        return static_cast<std::int32_t>(MappingData<From, To>::getNewBlockStateId(static_cast<std::uint32_t>(id)));
    }

    /** Every `BlockRuntimeId` inside `value`. */
    template <class T>
    static void rewriteBlockIds(T &value)
    {
        if constexpr (!carries_block_ids_v<T>) {
            return;
        }
        else if constexpr (std::is_same_v<T, bp::BlockRuntimeId>) {
            value = bp::BlockRuntimeId{MappingData<From, To>::getNewBlockStateId(value)};
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

    /** The blocks of the chunks one packet carries, a hop at a time: each hop's ids, then the states
     * the hop works out from neighbouring blocks, read from these chunks or, for a neighbour they do
     * not hold, from the blocks the client was sent before. The client keeps these ones too.
     * @see ViaVersion BlockRewriter#handleChunk. */
    static void handleChunks(std::span<Chunk> chunks, BlockConnectionStorage &storage)
    {
        rewriteHops<From, To>(chunks, storage);
        if constexpr (connectsBlocks<From, To>()) {
            std::vector<bp::BlockPos> stored;
            for (const auto &chunk : chunks) {
                for (const auto &sub_chunk : chunk.getSubChunks()) {
                    if (const auto *blocks = sub_chunk.palette(0)) {
                        const bp::BlockPos pos{chunk.getX(), sub_chunk.getY(), chunk.getZ()};
                        storage.storeSection(pos, *blocks);
                        stored.push_back(pos);
                    }
                }
            }
            updateChunkSectionNeighbours(stored, storage);
        }
    }

    /** The id `To` gives the block `From` calls `id` at `pos`, with the states each hop works out
     * from the blocks the client was sent beside it. The client keeps it too. */
    static std::uint32_t rewriteBlock(const bp::BlockPos &pos, std::uint32_t id, BlockConnectionStorage &storage)
    {
        if constexpr (connectsBlocks<From, To>()) {
            const auto connected = connectBlock<From, To>(pos, id, storage);
            storage.store(pos, connected);
            return connected;
        }
        else {
            return MappingData<From, To>::getNewBlockStateId(id);
        }
    }

    /** Works out again the blocks the client was sent around the `changed` ones, and keeps those
     * whose states change to send later, as the server sends nothing for them. Redstone reads the
     * wire a block up or down on each side, so this reaches past the faces.
     * @see ViaVersion ConnectionData#update. */
    static void update(std::span<const bp::BlockPos> changed, BlockConnectionStorage &storage)
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
                reconnectBlock({x, y, z}, storage);
            }
        }
    }

    /** Works out again the kept blocks along the `stored` sub-chunks in the sub-chunks beside them,
     * which the client was sent first, with the stored ones' blocks counted as air, and keeps those
     * whose states change to send later.
     * @see ViaVersion ConnectionData.NeighbourUpdater#updateChunkSectionNeighbours. */
    static void updateChunkSectionNeighbours(std::span<const bp::BlockPos> stored, BlockConnectionStorage &storage)
    {
        std::set<std::array<int, 3>> fresh;
        for (const auto &pos : stored) {
            fresh.insert({pos.x, pos.y, pos.z});
        }
        const auto edge = [](int side) {
            return side < 0 ? std::pair{15, 15} : side > 0 ? std::pair{0, 0} : std::pair{0, 15};
        };
        for (const auto &pos : stored) {
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dz = -1; dz <= 1; ++dz) {
                        const bp::BlockPos beside{pos.x + dx, pos.y + dy, pos.z + dz};
                        if (fresh.contains({beside.x, beside.y, beside.z})) {
                            continue;
                        }
                        const auto *blocks = storage.getSubChunk(beside);
                        if (blocks == nullptr ||
                            std::ranges::none_of(std::views::iota(std::size_t{0}, blocks->size()), [&](std::size_t i) {
                                return connectsBlock<From, To>(blocks->idByIndex(i));
                            })) {
                            continue;
                        }
                        const auto [x0, x1] = edge(dx);
                        const auto [y0, y1] = edge(dy);
                        const auto [z0, z1] = edge(dz);
                        for (int x = x0; x <= x1; ++x) {
                            for (int y = y0; y <= y1; ++y) {
                                for (int z = z0; z <= z1; ++z) {
                                    reconnectBlock({beside.x * 16 + x, beside.y * 16 + y, beside.z * 16 + z}, storage);
                                }
                            }
                        }
                    }
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

    /** Works out again the kept block at `pos`, and keeps it to send later if its states change. */
    static void reconnectBlock(const bp::BlockPos &pos, BlockConnectionStorage &storage)
    {
        const auto sent = storage.get(pos);
        if (!sent) {
            return;
        }
        const auto connected = connectBlock<From, To>(pos, MappingData<To, From>::getNewBlockStateId(*sent), storage);
        if (connected != *sent) {
            storage.setUnsent(pos, connected);
        }
    }

    /** Whether a hop between `A` and `B` gives the block `To` calls `id` states from its neighbours. */
    template <int A, int B>
    static bool connectsBlock(std::uint32_t id)
    {
        if constexpr (A == B) {
            return false;
        }
        else {
            constexpr int next = step(A, B);
            if constexpr (HasConnections<A, next>) {
                if (ConnectionData<A, next>::connects(MappingData<To, next>::getNewBlockStateId(id))) {
                    return true;
                }
            }
            return connectsBlock<next, B>(id);
        }
    }

    /** The block `To` has at `pos` that the client was sent, as the hop to `B` calls it. */
    template <int B>
    static std::optional<std::uint32_t> getSent(const BlockConnectionStorage &storage, const bp::BlockPos &pos)
    {
        const auto id = storage.get(pos);
        return id ? std::optional{MappingData<To, B>::getNewBlockStateId(*id)} : std::nullopt;
    }

    template <int A, int B>
    static void rewriteHops(std::span<Chunk> chunks, const BlockConnectionStorage &storage)
    {
        if constexpr (A != B) {
            constexpr int next = step(A, B);
            for (auto &chunk : chunks) {
                for (auto &sub_chunk : chunk.getSubChunks()) {
                    for (auto &palette : sub_chunk.palettes()) {
                        palette.replaceIds(&MappingData<A, next>::getNewBlockStateId);
                    }
                }
            }
            if constexpr (HasConnections<A, next>) {
                connectBlocks<A, next>(chunks, storage);
            }
            rewriteHops<next, B>(chunks, storage);
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
            auto mapped = MappingData<A, next>::getNewBlockStateId(id);
            if constexpr (HasConnections<A, next>) {
                if (ConnectionData<A, next>::connects(mapped)) {
                    const auto blocks = [&](const bp::BlockPos &at) {
                        return getSent<next>(storage, at);
                    };
                    mapped = ConnectionData<A, next>::connect(pos, mapped, blocks);
                }
            }
            return connectBlock<next, B>(pos, mapped, storage);
        }
    }

    /** @see ViaVersion ConnectionData#connectBlocks. */
    template <int A, int B>
    static void connectBlocks(std::span<Chunk> chunks, const BlockConnectionStorage &storage)
    {
        std::map<std::tuple<int, int, int>, const DataPalette *> sections;
        for (const auto &chunk : chunks) {
            for (const auto &sub_chunk : chunk.getSubChunks()) {
                if (const auto *blocks = sub_chunk.palette(0)) {
                    sections.emplace(std::tuple{chunk.getX(), sub_chunk.getY(), chunk.getZ()}, blocks);
                }
            }
        }
        const auto blocks = [&](const bp::BlockPos &pos) -> std::optional<std::uint32_t> {
            const auto it = sections.find({pos.x >> 4, pos.y >> 4, pos.z >> 4});
            if (it == sections.end()) {
                return getSent<B>(storage, pos);
            }
            return it->second->idAt(SubChunk::index(pos.x & 15, pos.y & 15, pos.z & 15));
        };

        std::vector<std::tuple<DataPalette *, std::uint16_t, std::uint32_t>> changes;
        for (auto &chunk : chunks) {
            for (auto &sub_chunk : chunk.getSubChunks()) {
                auto *const layer = sub_chunk.palette(0);
                if (layer == nullptr) {
                    continue;
                }
                layer->forEachMatchingCoordinate(&ConnectionData<A, B>::connects, [&](std::uint16_t index) {
                    const auto id = *layer->idAt(index);
                    const bp::BlockPos pos{chunk.getX() * 16 + SubChunk::xFromIndex(index),
                                           sub_chunk.getY() * 16 + SubChunk::yFromIndex(index),
                                           chunk.getZ() * 16 + SubChunk::zFromIndex(index)};
                    const auto connected = ConnectionData<A, B>::connect(pos, id, blocks);
                    if (connected != id) {
                        changes.emplace_back(layer, index, connected);
                    }
                });
            }
        }
        for (const auto &[layer, index, id] : changes) {
            layer->setIdAt(index, id);
        }
    }
};

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
        bp::BinaryReader in{packet.serialized_chunk};
        auto chunk = ChunkType::read(in, packet.pos.x, 0, packet.pos.z, packet.sub_chunks_count);
        if (!chunk) {
            return std::unexpected(chunk.error());
        }
        BlockRewriter<From, To>::handleChunks(std::span{&*chunk, 1}, storage);
        std::string data;
        bp::BinaryWriter out{data};
        ChunkType::write(out, *chunk);
        packet.serialized_chunk = std::move(data);
        return {};
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::SubChunkPacket)> {
    static std::expected<void, std::error_code> rewrite(bp::SubChunkPacket_<From> &packet,
                                                        BlockConnectionStorage &storage)
    {
        std::vector<Chunk> chunks;
        std::vector<std::string *> entries;
        for (auto &entry : packet.sub_chunk_data) {
            if (!entry.serialized_sub_chunk || entry.serialized_sub_chunk->empty()) {
                continue;
            }
            const auto &center = packet.center_pos;
            const auto &offset = entry.sub_chunk_pos_offset;
            bp::BinaryReader in{*entry.serialized_sub_chunk};
            auto chunk = ChunkType::read(in, center.x + offset.x, center.y + offset.y, center.z + offset.z, 1);
            if (!chunk) {
                return std::unexpected(chunk.error());
            }
            chunks.push_back(std::move(*chunk));
            entries.push_back(&*entry.serialized_sub_chunk);
        }
        BlockRewriter<From, To>::handleChunks(chunks, storage);
        for (std::size_t i = 0; i < chunks.size(); ++i) {
            std::string data;
            bp::BinaryWriter out{data};
            ChunkType::write(out, chunks[i]);
            *entries[i] = std::move(data);
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
        packet.runtime_id = bp::BlockRuntimeId{MappingData<From, To>::getNewBlockStateId(packet.runtime_id)};
        return;
    }
    const bp::BlockPos pos{packet.pos.x, packet.pos.y, packet.pos.z};
    packet.runtime_id = bp::BlockRuntimeId{BlockRewriter<From, To>::rewriteBlock(pos, packet.runtime_id, storage)};
    BlockRewriter<From, To>::update(std::span{&pos, 1}, storage);
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
        if constexpr (connectsBlocks<From, To>()) {
            for (const auto &block : packet.blocks_changed.standards) {
                storage.store({block.pos.x, block.pos.y, block.pos.z},
                              MappingData<From, To>::getNewBlockStateId(block.runtime_id));
            }
        }
        std::vector<bp::BlockPos> changed;
        for (auto &block : packet.blocks_changed.standards) {
            const bp::BlockPos pos{block.pos.x, block.pos.y, block.pos.z};
            block.runtime_id =
                bp::BlockRuntimeId{BlockRewriter<From, To>::rewriteBlock(pos, block.runtime_id, storage)};
            changed.push_back(pos);
        }
        BlockRewriter<From, To>::update(changed, storage);
        for (auto &block : packet.blocks_changed.extras) {
            block.runtime_id = bp::BlockRuntimeId{MappingData<From, To>::getNewBlockStateId(block.runtime_id)};
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
