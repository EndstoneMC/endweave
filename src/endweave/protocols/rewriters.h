#pragma once

#include "endweave/protocol/context.h"
#include "endweave/protocol/mapping.h"
#include "endweave/protocol/rewrite.h"
#include "endweave/rewriter/block.h"
#include "endweave/rewriter/enum.h"

#include <algorithm>
#include <bedrock/protocol/actor.h>
#include <bedrock/protocol/effect.h>
#include <bedrock/protocol/enum.hpp>
#include <bedrock/protocol/game.h>
#include <bedrock/protocol/inventory.h>
#include <bedrock/protocol/network.h>
#include <bedrock/protocol/pack.h>
#include <bedrock/protocol/packets_fwd.hpp>
#include <bedrock/protocol/player.h>
#include <bedrock/protocol/sound.h>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace bp = bedrock::protocol;

namespace endweave {

namespace detail {

// ENDWEAVE: To gates behind these experiments what From ships as base game, so the client is told
// they are on, as a world that runs them is.
template <int From, int To>
void enableExperiments(bp::Experiments &experiments)
{
    for (const auto name : MappingData<From, To>::getExperiments()) {
        const auto toggle = std::ranges::find(experiments.toggles, name, &bp::ExperimentToggle::name);
        if (toggle != experiments.toggles.end()) {
            toggle->enabled = true;
        }
        else {
            experiments.toggles.push_back({.name = std::string(name), .enabled = true});
        }
        experiments.experiments_ever_toggled = true;
    }
}

} // namespace detail

// ENDWEAVE: the checksum is taken over the server's block registry, which no translation can
// reproduce for the other side. Zero is the "do not check" value at every version, so this holds
// for every pair.
template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::StartGame)> {
    static void rewrite(bp::StartGamePacket_<From> &packet)
    {
        packet.server_block_type_registry_checksum = 0;
        // ENDWEAVE: a block type To lacks reaches the client as its fallback, so the server's definition
        // of it is left out.
        std::erase_if(packet.block_properties, [](const auto &property) {
            return MappingData<From, To>::getNewBlockName(property.name) != property.name;
        });
        detail::enableExperiments<From, To>(packet.settings.experiments);
    }
};

template <int From, int To>
    requires(MappingData<From, To>::enablesExperiments())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::ResourcePackStack)> {
    static void rewrite(bp::ResourcePackStackPacket_<From> &packet)
    {
        detail::enableExperiments<From, To>(packet.experiments);
    }
};

namespace detail {

/** Whether the numbers actor data carries untyped, its keys and its heartbeat sound, differ between
 * `From` and `To`. */
template <int From, int To>
inline constexpr bool actor_data_renumbered_v =
    numbers_apart_v<bp::ActorDataIDs, From, To> || numbers_apart_v<bp::LevelSoundEvent, From, To>;

// ENDWEAVE: this key holds a LevelSoundEvent in From's numbers, whose Undefined, "no heartbeat
// sound", is renumbered every version. Left alone, an actor meaning no sound names a real one at
// the other end and plays it every HEARTBEAT_INTERVAL_TICKS. A sound To has no number for lands on
// Undefined.
template <int From, int To>
void rewriteHeartbeatSound(std::vector<bp::DataItemEntry_<From>> &entries)
{
    constexpr auto undefined = *bp::enum_integer_<To>(bp::LevelSoundEvent::Undefined);
    for (auto &entry : entries) {
        auto *const sound = std::get_if<bp::DataItemIntPayload_<From>>(&entry.payload);
        if (sound == nullptr || entry.id != bp::enum_integer_<From>(bp::ActorDataIDs::HeartbeatSoundEvent)) {
            continue;
        }
        const auto event = bp::enum_cast_<From, bp::LevelSoundEvent>(static_cast<std::uint32_t>(sound->value));
        sound->value = static_cast<std::int32_t>(event ? bp::enum_integer_<To>(*event).value_or(undefined) : undefined);
    }
}

// ENDWEAVE: the keys are in From's numbers, and a key To has no number for is left out. A key past
// every ActorDataIDs value names nothing at either end and is carried as it is.
template <int From, int To>
void rewriteActorDataKeys(std::vector<bp::DataItemEntry_<From>> &entries)
{
    using Key = std::underlying_type_t<bp::ActorDataIDs>;
    std::erase_if(entries, [](auto &entry) {
        if (!std::in_range<Key>(entry.id)) {
            return false;
        }
        const auto key = bp::enum_cast_<From, bp::ActorDataIDs>(static_cast<Key>(entry.id));
        const auto number = key ? bp::enum_integer_<To>(*key) : std::nullopt;
        if (number) {
            entry.id = *number;
        }
        return !number;
    });
}

/** Gives the numbers actor data carries untyped `To`'s, its keys last. */
template <int From, int To>
void renumberActorData(std::vector<bp::DataItemEntry_<From>> &entries)
{
    if constexpr (actor_data_renumbered_v<From, To>) {
        rewriteHeartbeatSound<From, To>(entries);
        rewriteActorDataKeys<From, To>(entries);
    }
}

// ENDWEAVE: this key packs up to eight visible effects in seven bits each, the id above the ambient
// bit. An effect the client lacks is left out, as its MobEffect is.
template <int From, int To>
void rewriteVisibleMobEffects(std::vector<bp::DataItemEntry_<From>> &entries)
{
    for (auto &entry : entries) {
        if (entry.id != bp::enum_integer_<From>(bp::ActorDataIDs::VisibleMobEffects)) {
            continue;
        }
        if (auto *const effects = std::get_if<bp::DataItemInt64Payload_<From>>(&entry.payload)) {
            std::uint64_t kept = 0;
            int shift = 0;
            for (auto packed = static_cast<std::uint64_t>(effects->value); packed != 0; packed >>= 7) {
                const std::uint64_t effect = packed & 0x7F;
                if (MappingData<From, To>::getNewMobEffectId(static_cast<std::int32_t>(effect >> 1))) {
                    kept |= effect << shift;
                    shift += 7;
                }
            }
            effects->value = static_cast<std::int64_t>(kept);
        }
    }
}

} // namespace detail

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::AddActor)> {
    static void rewrite(bp::AddActorPacket_<From> &packet)
    {
        if constexpr (MappingData<From, To>::mapsMobEffects()) {
            detail::rewriteVisibleMobEffects<From, To>(packet.data.data);
        }
        BlockRewriter<From, To>::rewriteActorData(packet.data.data, packet.actor_type == "minecraft:falling_block");
        detail::renumberActorData<From, To>(packet.data.data);
        if constexpr (MappingData<From, To>::mapsEntities()) {
            // ENDWEAVE: an actor type the client lacks is spawned as its fallback, which is given none
            // of the properties the server synced for the type it replaces.
            if (const auto name = MappingData<From, To>::getNewEntityName(packet.actor_type);
                name != packet.actor_type) {
                packet.actor_type = std::string(name);
                packet.synched_properties = {};
            }
        }
    }
};

// ENDWEAVE: an actor type the client lacks reaches it as its fallback, so it is left out of the
// types the client is told of.
template <int From, int To>
    requires(MappingData<From, To>::mapsEntities())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::AvailableActorIDList)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::AvailableActorIDList)> &packet)
    {
        bp::CompoundTag &identifiers = packet.identifier_list;
        const auto idlist = identifiers.find("idlist");
        auto *const entries = idlist != identifiers.end() ? idlist->second.get_if<bp::ListTag>() : nullptr;
        if (entries == nullptr) {
            return;
        }
        const auto lacked = std::remove_if(entries->begin(), entries->end(), [](const bp::Tag &entry) {
            const auto *const id = entry.contains("id") ? entry.at("id").get_if<bp::StringTag>() : nullptr;
            return id != nullptr && MappingData<From, To>::getNewEntityName(id->value()) != id->value();
        });
        entries->erase(lacked, entries->end());
    }
};

// ENDWEAVE: the client has no actor type to give these properties to.
template <int From, int To>
    requires(MappingData<From, To>::mapsEntities())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::SyncActorProperty)> {
    static void rewrite(
        Context<bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::SyncActorProperty)>> &ctx)
    {
        const bp::CompoundTag &properties = ctx.out().property_data;
        const auto type = properties.find("type");
        const auto *const name = type != properties.end() ? type->second.get_if<bp::StringTag>() : nullptr;
        if (name != nullptr && MappingData<From, To>::getNewEntityName(name->value()) != name->value()) {
            ctx.cancel();
        }
    }
};

template <int From, int To>
    requires detail::actor_data_renumbered_v<From, To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::AddItemActor)> {
    static void rewrite(bp::AddItemActorPacket_<From> &packet)
    {
        detail::renumberActorData<From, To>(packet.data.data);
    }
};

template <int From, int To>
    requires(detail::actor_data_renumbered_v<From, To> || MappingData<From, To>::mapsMobEffects())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::AddPlayer)> {
    static void rewrite(bp::AddPlayerPacket_<From> &packet)
    {
        if constexpr (MappingData<From, To>::mapsMobEffects()) {
            detail::rewriteVisibleMobEffects<From, To>(packet.unpack.data);
        }
        detail::renumberActorData<From, To>(packet.unpack.data);
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::SetActorData)> {
    static void rewrite(bp::SetActorDataPacket_<From> &packet)
    {
        if constexpr (MappingData<From, To>::mapsMobEffects()) {
            detail::rewriteVisibleMobEffects<From, To>(packet.packed_items.data);
        }
        BlockRewriter<From, To>::rewriteActorData(packet.packed_items.data, false);
        detail::renumberActorData<From, To>(packet.packed_items.data);
    }
};

// ENDWEAVE: a container the client has no name for was never opened there, so it is left out of the
// ones to clean up, rather than the packet dropping the rest with it.
template <int From, int To>
    requires(detail::loses<bp::ContainerEnumName, From, To>())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::ContainerRegistryCleanup)> {
    static void rewrite(
        bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::ContainerRegistryCleanup)> &packet)
    {
        std::erase_if(packet.removed_containers, [](const bp::FullContainerName &container) {
            return !bp::enum_contains_<To>(container.name);
        });
    }
};

// ENDWEAVE: an effect the client has no MobEffect for is left off it, as ViaBackwards leaves off
// breath_of_the_nautilus.
template <int From, int To>
    requires(MappingData<From, To>::mapsMobEffects())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::MobEffect)> {
    static void rewrite(Context<bp::MobEffectPacket_<From>> &ctx)
    {
        if (!MappingData<From, To>::getNewMobEffectId(ctx.out().effect_id)) {
            ctx.cancel();
        }
    }
};

} // namespace endweave
