#include "endweave/listener.h"

#include "endweave/protocol/packet_ids.h"

#include <string_view>

namespace endweave {

std::string addressKey(const endstone::SocketAddress &address)
{
    return address.getHostname() + ":" + std::to_string(address.getPort());
}

template <class Event>
void PacketListener::translate(Direction direction, Event &event)
{
    UserConnection &connection = connections_->getOrCreate(addressKey(event.getAddress()));
    connection.touch();

    const std::string_view payload = event.getPayload();
    auto result =
        connection.getProtocolInfo().getPipeline().transform(direction, event.getPacketId(), connection, payload);
    if (!result) {
        connection.reportTranslationError(event.getPacketId(), result.error());
        event.cancel();
        return;
    }
    if (!result->has_value()) {
        event.cancel();
        return;
    }
    // setPayload() compares by pointer identity, so calling it at all forces the rewrite path.
    // The view aliases the input unless a stage actually rewrote.
    if ((*result)->data() != payload.data()) {
        event.setPayload(**result);
    }
}

void PacketListener::onPacketReceive(endstone::PacketReceiveEvent &event)
{
    translate(Direction::Serverbound, event);
    if (event.getPacketId() == static_cast<int>(MinecraftPacketIds::Disconnect)) {
        connections_->onDisconnect(addressKey(event.getAddress()));
    }
}

void PacketListener::onPacketSend(endstone::PacketSendEvent &event)
{
    translate(Direction::Clientbound, event);
    if (event.getPacketId() == static_cast<int>(MinecraftPacketIds::Disconnect)) {
        connections_->onDisconnect(addressKey(event.getAddress()));
    }
}

void PacketListener::onPlayerQuit(endstone::PlayerQuitEvent &event)
{
    connections_->onDisconnect(addressKey(event.getPlayer().getAddress()));
}

} // namespace endweave
