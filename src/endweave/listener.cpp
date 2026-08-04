#include "endweave/listener.h"

#include <cstdint>
#include <protocol/network.h>
#include <string>
#include <string_view>

namespace bp = bedrock::protocol;

namespace endweave {
namespace {

constexpr int kDisconnectPacketId = static_cast<int>(bp::MinecraftPacketIds::DISCONNECT);
constexpr int kRequestNetworkSettingsPacketId = static_cast<int>(bp::MinecraftPacketIds::REQUEST_NETWORK_SETTINGS);

/** The client announces its protocol before anything else on the connection is
 * readable, which is why this packet carries nothing else. */
ProtocolVersion readClientVersion(std::string_view payload)
{
    bp::BinaryReader in{payload};
    const auto packet = bp::deserialize<bp::RequestNetworkSettingsPacket>(in);
    if (!packet) {
        return ProtocolVersion::UNKNOWN;
    }
    return ProtocolVersions::getProtocolVersion(packet->client_network_version);
}

/** The server compares the announced protocol against its own and disconnects the client
 * outright when they differ, before any packet this plugin could translate. So the
 * version it sees has to be its own. */
std::string announceServerVersion()
{
    bp::RequestNetworkSettingsPacket packet;
    packet.client_network_version = static_cast<std::int32_t>(ProtocolVersions::SERVER_VERSION);
    std::string payload;
    bp::BinaryWriter out{payload};
    bp::serialize(out, packet);
    return payload;
}

} // namespace

template <class Event>
void PacketListener::translate(Event &event, const PacketHandlers &handlers)
{
    const int id = event.getPacketId();
    if (handlers.isCancelled(id)) {
        event.setCancelled(true);
        return;
    }

    const PacketHandler handler = handlers.get(id);
    if (handler == nullptr) {
        return;
    }

    std::string translated;
    bp::BinaryWriter out{translated};
    bp::BinaryReader in{event.getPayload()};
    if (const auto result = handler(in, out); !result) {
        logger_->warning("Dropping packet {}: {}.", id, result.error().message());
        event.setCancelled(true);
        return;
    }
    event.setPayload(translated);
}

template <class Event>
UserConnection *PacketListener::touch(Event &event)
{
    const endstone::SocketAddress address = event.getAddress();
    if (event.getPacketId() == kDisconnectPacketId) {
        connections_->onDisconnect(address);
        return nullptr;
    }
    UserConnection &connection = connections_->getOrCreate(address);
    connection.touch();
    return &connection;
}

void PacketListener::onPacketReceive(endstone::PacketReceiveEvent &event)
{
    UserConnection *connection = touch(event);
    if (connection == nullptr) {
        return;
    }

    if (event.getPacketId() == kRequestNetworkSettingsPacketId) {
        const ProtocolVersion version = readClientVersion(event.getPayload());
        if (version == ProtocolVersion::UNKNOWN) {
            logger_->info("{} speaks a protocol endweave does not translate; passing it through.",
                          connection->getAddress().getHostname());
            return;
        }
        connection->setClientVersion(version);
        if (version != ProtocolVersions::SERVER_VERSION) {
            event.setPayload(announceServerVersion());
        }
        logger_->info("{} connected on protocol {}.", connection->getAddress().getHostname(),
                      static_cast<int>(version));
        return;
    }

    translate(event, connection->getServerboundHandlers());
}

void PacketListener::onPacketSend(endstone::PacketSendEvent &event)
{
    UserConnection *connection = touch(event);
    if (connection == nullptr) {
        return;
    }
    translate(event, connection->getClientboundHandlers());
}

void PacketListener::onPlayerQuit(endstone::PlayerQuitEvent &event)
{
    connections_->onDisconnect(event.getPlayer().getAddress());
}

} // namespace endweave
