#include "endweave/listener.h"

#include <protocol/network.h>

namespace endweave {
namespace {

constexpr int kDisconnectPacketId = static_cast<int>(bedrock::protocol::MinecraftPacketIds::DISCONNECT);

} // namespace

template <class Event>
void PacketListener::handle(Event &event)
{
    const endstone::SocketAddress address = event.getAddress();
    if (event.getPacketId() == kDisconnectPacketId) {
        connections_->onDisconnect(address);
        return;
    }
    connections_->getOrCreate(address).touch();
}

void PacketListener::onPacketReceive(endstone::PacketReceiveEvent &event)
{
    handle(event);
}

void PacketListener::onPacketSend(endstone::PacketSendEvent &event)
{
    handle(event);
}

void PacketListener::onPlayerQuit(endstone::PlayerQuitEvent &event)
{
    connections_->onDisconnect(event.getPlayer().getAddress());
}

} // namespace endweave
