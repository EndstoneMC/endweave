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
constexpr int kLoginPacketId = static_cast<int>(bp::MinecraftPacketIds::LOGIN);
constexpr int kPacketViolationWarningPacketId = static_cast<int>(bp::MinecraftPacketIds::PACKET_VIOLATION_WARNING);

// LoginPacket is one type at every version. The violation warning is not -- it names the
// offending packet with MinecraftPacketIds, which gained members -- but it is the same
// shape across the versions this plugin supports, so one decode serves both directions.
// The assert fails the build if that stops being true, since then it would have to be
// decoded at the sender's version rather than the latest.
static_assert(std::is_same_v<bp::PacketViolationWarningPacket_<static_cast<int>(ProtocolVersion::v26_30)>,
                             bp::PacketViolationWarningPacket_<static_cast<int>(ProtocolVersion::v26_40)>>);

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

/** Login repeats the protocol version, and the server checks it a second time, so the
 * handshake rewrite alone is not enough to get a client past the door. */
std::string rewriteLoginVersion(std::string_view payload)
{
    bp::BinaryReader in{payload};
    auto packet = bp::deserialize<bp::LoginPacket>(in);
    if (!packet) {
        return {};
    }
    packet->client_network_version = static_cast<std::int32_t>(ProtocolVersions::SERVER_VERSION);
    std::string rewritten;
    bp::BinaryWriter out{rewritten};
    bp::serialize(out, *packet);
    return rewritten;
}

/** Whichever side could not parse a packet says so with this, naming the offending id.
 * It is the only signal that points at a mistranslation rather than at its symptom, so
 * it is surfaced rather than forwarded silently. */
void logViolation(endstone::Logger &logger, std::string_view payload, std::string_view reporter)
{
    bp::BinaryReader in{payload};
    const auto packet = bp::deserialize<bp::PacketViolationWarningPacket>(in);
    if (!packet) {
        logger.warning("{} reported a packet violation, but the warning itself did not decode.", reporter);
        return;
    }
    logger.warning("{} reported a packet violation: {}/{} on {}, context={:?}", reporter,
                   bp::enum_name(packet->violation_type), bp::enum_name(packet->violation_severity),
                   bp::enum_name(packet->violating_packet_id), packet->violation_context);
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
        logger_->warning("Dropping {}: {}.", packetLabel(id), result.error().message());
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

template <class Event>
void PacketListener::log(std::string_view stage, Event &event, const UserConnection &connection,
                         std::string_view direction) const
{
    debug_.logPacket(stage, connection.getAddress().getHostname(), direction, event.getPacketId(),
                     static_cast<int>(connection.getClientVersion()), event.getPayload().size());
}

void PacketListener::receive(endstone::PacketReceiveEvent &event, UserConnection &connection)
{
    if (event.getPacketId() == kRequestNetworkSettingsPacketId) {
        const ProtocolVersion version = readClientVersion(event.getPayload());
        if (version == ProtocolVersion::UNKNOWN) {
            logger_->info("{} speaks a protocol endweave does not translate; passing it through.",
                          connection.getAddress().getHostname());
            return;
        }
        connection.setClientVersion(version);
        if (version != ProtocolVersions::SERVER_VERSION) {
            event.setPayload(announceServerVersion());
        }
        logger_->info("{} connected on protocol {}.", connection.getAddress().getHostname(), static_cast<int>(version));
        return;
    }

    if (event.getPacketId() == kLoginPacketId && connection.getClientVersion() != ProtocolVersions::SERVER_VERSION) {
        std::string rewritten = rewriteLoginVersion(event.getPayload());
        if (rewritten.empty()) {
            logger_->warning("{} sent a login that did not decode; leaving it untouched.",
                             connection.getAddress().getHostname());
            return;
        }
        event.setPayload(rewritten);
        return;
    }

    if (event.getPacketId() == kPacketViolationWarningPacketId) {
        logViolation(*logger_, event.getPayload(), "The client");
        return;
    }

    translate(event, connection.getServerboundHandlers());
}

void PacketListener::send(endstone::PacketSendEvent &event, UserConnection &connection)
{
    if (event.getPacketId() == kPacketViolationWarningPacketId) {
        logViolation(*logger_, event.getPayload(), "The server");
        return;
    }

    translate(event, connection.getClientboundHandlers());
}

void PacketListener::onPacketReceive(endstone::PacketReceiveEvent &event)
{
    UserConnection *connection = touch(event);
    if (connection == nullptr) {
        return;
    }
    log("PRE ", event, *connection, "SERVERBOUND");
    receive(event, *connection);
    log("POST", event, *connection, "SERVERBOUND");
}

void PacketListener::onPacketSend(endstone::PacketSendEvent &event)
{
    UserConnection *connection = touch(event);
    if (connection == nullptr) {
        return;
    }
    log("PRE ", event, *connection, "CLIENTBOUND");
    send(event, *connection);
    log("POST", event, *connection, "CLIENTBOUND");
}

void PacketListener::onPlayerQuit(endstone::PlayerQuitEvent &event)
{
    connections_->onDisconnect(event.getPlayer().getAddress());
}

} // namespace endweave
