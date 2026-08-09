#pragma once

#include "endweave/protocol/rewrite.h"

#include <protocol/game.h>
#include <protocol/network.h>

namespace bp = bedrock::protocol;

namespace endweave {

// ENDWEAVE: the checksum is taken over the server's block registry, which no translation can
// reproduce for the other side. Zero is the "do not check" value at every version, so this holds
// wherever the packet sits in the chain.
template <ProtocolVersion V>
struct Rewriter<V, static_cast<int>(bp::MinecraftPacketIds::START_GAME)> {
    static void rewrite(bp::StartGamePacket_<static_cast<int>(V)> &packet)
    {
        packet.server_block_type_registry_checksum = 0;
    }
};

} // namespace endweave
