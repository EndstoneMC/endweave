#pragma once

#include "endweave/protocol/rewrite.h"

#include <bedrock/protocol/actor.h>
#include <bedrock/protocol/enum.hpp>
#include <bedrock/protocol/game.h>
#include <bedrock/protocol/network.h>
#include <bedrock/protocol/player.h>
#include <bedrock/protocol/sound.h>
#include <cstdint>
#include <variant>
#include <vector>

namespace bp = bedrock::protocol;

namespace endweave {

// ENDWEAVE: the checksum is taken over the server's block registry, which no translation can
// reproduce for the other side. Zero is the "do not check" value at every version, so this holds
// for every pair.
template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::StartGame)> {
    static void rewrite(bp::StartGamePacket_<From> &packet)
    {
        packet.server_block_type_registry_checksum = 0;
    }
};

namespace detail {

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

} // namespace detail

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::AddActor)> {
    static void rewrite(bp::AddActorPacket_<From> &packet)
    {
        detail::rewriteActorData<From, To>(packet.data.data);
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::AddItemActor)> {
    static void rewrite(bp::AddItemActorPacket_<From> &packet)
    {
        detail::rewriteActorData<From, To>(packet.data.data);
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::AddPlayer)> {
    static void rewrite(bp::AddPlayerPacket_<From> &packet)
    {
        detail::rewriteActorData<From, To>(packet.unpack.data);
    }
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::SetActorData)> {
    static void rewrite(bp::SetActorDataPacket_<From> &packet)
    {
        detail::rewriteActorData<From, To>(packet.packed_items.data);
    }
};

} // namespace endweave
