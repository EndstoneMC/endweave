#pragma once

#include "endweave/protocol/context.h"
#include "endweave/protocol/mapping.h"
#include "endweave/protocol/rewrite.h"
#include "endweave/rewriter/block.h"

#include <algorithm>
#include <bedrock/protocol/actor.h>
#include <bedrock/protocol/effect.h>
#include <bedrock/protocol/enum.hpp>
#include <bedrock/protocol/game.h>
#include <bedrock/protocol/network.h>
#include <bedrock/protocol/pack.h>
#include <bedrock/protocol/player.h>
#include <bedrock/protocol/sound.h>
#include <cstdint>
#include <string>
#include <type_traits>
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

template <int From, int To>
inline constexpr bool sound_renumbered_v = !std::is_same_v<bp::LevelSoundEvent_<From>, bp::LevelSoundEvent_<To>>;

// ENDWEAVE: this key holds a LevelSoundEvent, whose Undefined sentinel is renumbered every
// version. Left alone, an actor meaning "no heartbeat sound" names a real one at the other end
// and plays it every HEARTBEAT_INTERVAL_TICKS. The transforms carry the number across untouched,
// so it still reads in From's numbering here and one lookup reaches To's exactly.
template <int From, int To>
void rewriteActorData(std::vector<bp::DataItemEntry_<From>> &entries)
{
    using FromSound = bp::LevelSoundEvent_<From>;
    using ToSound = bp::LevelSoundEvent_<To>;
    for (auto &entry : entries) {
        if (entry.id != static_cast<std::uint32_t>(bp::ActorDataIDs_<From>::HeartbeatSoundEvent)) {
            continue;
        }
        if (auto *const sound = std::get_if<bp::DataItemIntPayload_<From>>(&entry.payload)) {
            const auto name = bp::enum_name(static_cast<FromSound>(sound->value));
            sound->value = static_cast<std::int32_t>(bp::enum_cast<ToSound>(name).value_or(ToSound::Undefined));
        }
    }
}

// ENDWEAVE: this key packs up to eight visible effects in seven bits each, the id above the ambient
// bit. An effect the client lacks is left out, as its MobEffect is.
template <int From, int To>
void rewriteVisibleMobEffects(std::vector<bp::DataItemEntry_<From>> &entries)
{
    for (auto &entry : entries) {
        if (entry.id != static_cast<std::uint32_t>(bp::ActorDataIDs_<From>::VisibleMobEffects)) {
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
        if constexpr (detail::sound_renumbered_v<From, To>) {
            detail::rewriteActorData<From, To>(packet.data.data);
        }
        if constexpr (MappingData<From, To>::mapsMobEffects()) {
            detail::rewriteVisibleMobEffects<From, To>(packet.data.data);
        }
        BlockRewriter<From, To>::rewriteActorData(packet.data.data, packet.actor_type == "minecraft:falling_block");
    }
};

template <int From, int To>
    requires detail::sound_renumbered_v<From, To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::AddItemActor)> {
    static void rewrite(bp::AddItemActorPacket_<From> &packet)
    {
        detail::rewriteActorData<From, To>(packet.data.data);
    }
};

template <int From, int To>
    requires(detail::sound_renumbered_v<From, To> || MappingData<From, To>::mapsMobEffects())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::AddPlayer)> {
    static void rewrite(bp::AddPlayerPacket_<From> &packet)
    {
        if constexpr (detail::sound_renumbered_v<From, To>) {
            detail::rewriteActorData<From, To>(packet.unpack.data);
        }
        if constexpr (MappingData<From, To>::mapsMobEffects()) {
            detail::rewriteVisibleMobEffects<From, To>(packet.unpack.data);
        }
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::SetActorData)> {
    static void rewrite(bp::SetActorDataPacket_<From> &packet)
    {
        if constexpr (detail::sound_renumbered_v<From, To>) {
            detail::rewriteActorData<From, To>(packet.packed_items.data);
        }
        if constexpr (MappingData<From, To>::mapsMobEffects()) {
            detail::rewriteVisibleMobEffects<From, To>(packet.packed_items.data);
        }
        BlockRewriter<From, To>::rewriteActorData(packet.packed_items.data, false);
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
