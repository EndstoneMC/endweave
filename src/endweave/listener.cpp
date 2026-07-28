#include "endweave/listener.h"

#include "endweave/protocol/packet_ids.h"

#include <string_view>

namespace endweave {

template <class Event>
void PacketListener::translate(Direction direction, Event &event)
{
    UserConnection &connection = connections_->getOrCreate(event.getAddress());
    connection.touch();

    const std::string_view payload = event.getPayload();
    auto result =
        connection.getProtocolInfo().getPipeline().transform(direction, event.getPacketId(), connection, payload);
    if (!result) {
        connection.reportTranslationError(event.getPacketId(), result.error());
        event.cancel();
        return;
    }
    const auto &translated = result.value();
    if (!translated.has_value()) {
        event.cancel();
        return;
    }
    // Only set the payload when a stage rewrote. The view aliases the input otherwise.
    if (translated.value().data() != payload.data()) {
        event.setPayload(translated.value());
    }
}

void PacketListener::onPacketReceive(endstone::PacketReceiveEvent &event)
{
    translate(Direction::Serverbound, event);
    if (event.getPacketId() == static_cast<int>(PacketIds::Disconnect)) {
        connections_->onDisconnect(event.getAddress());
    }
}

void PacketListener::onPacketSend(endstone::PacketSendEvent &event)
{
    translate(Direction::Clientbound, event);
    if (event.getPacketId() == static_cast<int>(PacketIds::Disconnect)) {
        connections_->onDisconnect(event.getAddress());
    }
}

void PacketListener::onPlayerQuit(endstone::PlayerQuitEvent &event)
{
    connections_->onDisconnect(event.getPlayer().getAddress());
}

} // namespace endweave
