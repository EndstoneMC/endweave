#pragma once

#include "endweave/protocol/block.h"
#include "endweave/protocol/rewrite.h"

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
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
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

    /** The palettes of the `count` network sub-chunks at the front of `data`. The packed indices are
     * copied as they are, and so is whatever follows the sub-chunks: biomes, border blocks and block
     * actors. */
    static std::expected<void, std::error_code> rewriteSubChunks(std::string &data, std::size_t count)
    {
        const auto malformed = [] {
            return std::unexpected(std::make_error_code(std::errc::protocol_error));
        };
        bp::BinaryReader in{data};
        std::string rewritten;
        rewritten.reserve(data.size());
        bp::BinaryWriter out{rewritten};
        for (std::size_t i = 0; i < count; ++i) {
            const auto version = in.read<std::uint8_t>();
            const auto storages = in.read<std::uint8_t>();
            if (!version || !storages || (*version != 8 && *version != 9)) {
                return malformed();
            }
            out.write<std::uint8_t>(*version);
            out.write<std::uint8_t>(*storages);
            if (*version == 9) {
                const auto y_index = in.read<std::uint8_t>();
                if (!y_index) {
                    return malformed();
                }
                out.write<std::uint8_t>(*y_index);
            }
            for (std::uint8_t storage = 0; storage < *storages; ++storage) {
                const auto header = in.read<std::uint8_t>();
                if (!header || (*header & 1) == 0) {
                    return malformed();
                }
                out.write<std::uint8_t>(*header);
                const unsigned bits = *header >> 1;
                // Repeats the storage before it.
                if (bits == 0x7F) {
                    continue;
                }
                if (bits != 0 && bits != 1 && bits != 2 && bits != 3 && bits != 4 && bits != 5 && bits != 6 &&
                    bits != 8 && bits != 16) {
                    return malformed();
                }
                const std::size_t per_word = bits == 0 ? 0 : 32 / bits;
                const std::size_t bytes = bits == 0 ? 0 : (4096 + per_word - 1) / per_word * 4;
                if (bytes > in.getUnreadLength()) {
                    return malformed();
                }
                out.writeRawBytes(in.getView().substr(in.getReadPointer(), bytes));
                in.setReadPointer(in.getReadPointer() + bytes);

                std::int32_t size = 1;
                if (bits != 0) {
                    const auto palette_size = in.readVarInt<std::int32_t>();
                    if (!palette_size || *palette_size <= 0 || *palette_size > (1 << bits)) {
                        return malformed();
                    }
                    size = *palette_size;
                    out.writeVarInt<std::int32_t>(size);
                }
                for (std::int32_t entry = 0; entry < size; ++entry) {
                    const auto id = in.readVarInt<std::int32_t>();
                    if (!id) {
                        return malformed();
                    }
                    out.writeVarInt<std::int32_t>(rewrite(*id));
                }
            }
        }
        out.writeRawBytes(in.getView().substr(in.getReadPointer()));
        data = std::move(rewritten);
        return {};
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
    static std::expected<void, std::error_code> rewrite(bp::LevelChunkPacket_<From> &packet)
    {
        // A request limit means the client asks for the sub-chunks, which then come in SubChunkPacket.
        if (packet.cache_enabled || packet.client_request_sub_chunk_limit.has_value()) {
            return {};
        }
        return BlockRewriter<From, To>::rewriteSubChunks(packet.serialized_chunk, packet.sub_chunks_count);
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::SubChunkPacket)> {
    static std::expected<void, std::error_code> rewrite(bp::SubChunkPacket_<From> &packet)
    {
        for (auto &entry : packet.sub_chunk_data) {
            if (!entry.serialized_sub_chunk || entry.serialized_sub_chunk->empty()) {
                continue;
            }
            if (auto rewritten = BlockRewriter<From, To>::rewriteSubChunks(*entry.serialized_sub_chunk, 1);
                !rewritten) {
                return rewritten;
            }
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
