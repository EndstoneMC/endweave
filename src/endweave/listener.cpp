#include "endweave/listener.h"

#include "endweave/protocol/manager.h"

#include <bedrock/protocol.hpp>
#include <string_view>

namespace endweave {
namespace {

constexpr int kDisconnectPacketId = static_cast<int>(bedrock::protocol::MinecraftPacketIds::DISCONNECT);

} // namespace

template <class Event>
void PacketListener::handle(Direction direction, Event &event)
{
    const int packet_id = event.getPacketId();
    const bool interesting = protocols_->isInteresting(packet_id);
    const bool disconnect = packet_id == kDisconnectPacketId;
    if (!interesting && !disconnect) {
        return; // no protocol registered anything for this id, so it is forwarded untouched
    }

    // getAddress() builds a SocketAddress by value, so it is read once and passed down.
    const endstone::SocketAddress address = event.getAddress();
    if (interesting) {
        translate(direction, event, address);
    }
    if (disconnect) {
        connections_->onDisconnect(address);
    }
}

template <class Event>
void PacketListener::translate(Direction direction, Event &event, const endstone::SocketAddress &address)
{
    UserConnection &connection = connections_->getOrCreate(address);
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
    handle(Direction::Serverbound, event);
}

void PacketListener::onPacketSend(endstone::PacketSendEvent &event)
{
    handle(Direction::Clientbound, event);
}

void PacketListener::onPlayerQuit(endstone::PlayerQuitEvent &event)
{
    connections_->onDisconnect(event.getPlayer().getAddress());
}

} // namespace endweave
