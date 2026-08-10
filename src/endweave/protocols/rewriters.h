#pragma once

#include "endweave/protocol/rewrite.h"

#include <bedrock/enum.hpp>
#include <cstdint>
#include <protocol/actor.h>
#include <protocol/game.h>
#include <protocol/movement.h>
#include <protocol/network.h>
#include <protocol/sound.h>
#include <variant>
#include <vector>

namespace bp = bedrock::protocol;

namespace endweave {

// ENDWEAVE: the checksum is taken over the server's block registry, which no translation can
// reproduce for the other side. Zero is the "do not check" value at every version, so this holds
// for every pair.
template <ProtocolVersion From, ProtocolVersion To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::START_GAME)> {
    static void rewrite(bp::StartGamePacket_<static_cast<int>(From)> &packet)
    {
        packet.server_block_type_registry_checksum = 0;
    }
};

namespace detail {

// ENDWEAVE: this key holds a LevelSoundEvent, whose Undefined sentinel is renumbered every
// version. Left alone, an actor meaning "no heartbeat sound" names a real one at the other end
// and plays it every HEARTBEAT_INTERVAL_TICKS. The transforms carry the number across untouched,
// so it still reads in From's numbering here and one lookup reaches To's exactly.
template <ProtocolVersion From, ProtocolVersion To>
void rewriteActorData(std::vector<bp::DataItemEntry_<static_cast<int>(From)>> &entries)
{
    using FromSound = bp::LevelSoundEvent_<static_cast<int>(From)>;
    using ToSound = bp::LevelSoundEvent_<static_cast<int>(To)>;
    for (auto &entry : entries) {
        if (entry.id != static_cast<std::uint32_t>(bp::ActorDataIDs::HEARTBEAT_SOUND_EVENT)) {
            continue;
        }
        if (auto *const sound = std::get_if<bp::DataItemIntPayload_<static_cast<int>(From)>>(&entry.payload)) {
            const auto name = bp::enum_name(static_cast<FromSound>(sound->value));
            sound->value = static_cast<std::int32_t>(bp::enum_cast<ToSound>(name).value_or(ToSound::UNDEFINED));
        }
    }
}

} // namespace detail

template <ProtocolVersion From, ProtocolVersion To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::ADD_ACTOR)> {
    static void rewrite(bp::AddActorPacket_<static_cast<int>(From)> &packet)
    {
        detail::rewriteActorData<From, To>(packet.data.data);
    }
};

template <ProtocolVersion From, ProtocolVersion To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::ADD_ITEM_ACTOR)> {
    static void rewrite(bp::AddItemActorPacket_<static_cast<int>(From)> &packet)
    {
        detail::rewriteActorData<From, To>(packet.data.data);
    }
};

template <ProtocolVersion From, ProtocolVersion To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::ADD_PLAYER)> {
    static void rewrite(bp::AddPlayerPacket_<static_cast<int>(From)> &packet)
    {
        detail::rewriteActorData<From, To>(packet.unpack.data);
    }
};

template <ProtocolVersion From, ProtocolVersion To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::SET_ACTOR_DATA)> {
    static void rewrite(bp::SetActorDataPacket_<static_cast<int>(From)> &packet)
    {
        detail::rewriteActorData<From, To>(packet.packed_items.data);
    }
};

} // namespace endweave
