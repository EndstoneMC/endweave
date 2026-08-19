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
std::string announceServerVersion(ProtocolVersion server_version)
{
    bp::RequestNetworkSettingsPacket packet;
    packet.client_network_version = static_cast<std::int32_t>(ProtocolVersions::networkVersion(server_version));
    std::string payload;
    bp::BinaryWriter out{payload};
    bp::serialize(out, packet);
    return payload;
}

/** Login repeats the protocol version, and the server checks it a second time, so the
 * handshake rewrite alone is not enough to get a client past the door. */
std::string rewriteLoginVersion(std::string_view payload, ProtocolVersion server_version)
{
    bp::BinaryReader in{payload};
    auto packet = bp::deserialize<bp::LoginPacket>(in);
    if (!packet) {
        return {};
    }
    packet->client_network_version = static_cast<std::int32_t>(ProtocolVersions::networkVersion(server_version));
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
void PacketListener::translate(Event &event, UserConnection &connection, const PacketHandlers &handlers)
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
    bool cancelled = false;
    if (const auto result = handler(connection, cancelled, in, out); !result) {
        logger_->warning("Dropping {}: {}.", packetLabel(id), result.error().message());
        event.setCancelled(true);
        return;
    }
    // A transform that could not say what the destination means dropped the packet on purpose,
    // which is not the failure above and is not worth a warning.
    if (cancelled) {
        logger_->debug("Cancelled {}: the destination cannot express it.", packetLabel(id));
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
                     static_cast<int>(connection.getClientVersion()), event.getPayload());
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
        connection.setClientVersion(version, server_version_);
        if (version != server_version_) {
            event.setPayload(announceServerVersion(server_version_));
        }
        logger_->info("{} connected on protocol {}.", connection.getAddress().getHostname(), static_cast<int>(version));
        return;
    }

    if (event.getPacketId() == kLoginPacketId && connection.getClientVersion() != server_version_) {
        std::string rewritten = rewriteLoginVersion(event.getPayload(), server_version_);
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

    translate(event, connection, connection.getServerboundHandlers());
}

void PacketListener::send(endstone::PacketSendEvent &event, UserConnection &connection)
{
    if (event.getPacketId() == kPacketViolationWarningPacketId) {
        logViolation(*logger_, event.getPayload(), "The server");
        return;
    }

    translate(event, connection, connection.getClientboundHandlers());
}

void PacketListener::onPacketReceive(endstone::PacketReceiveEvent &event)
{
    UserConnection *connection = touch(event);
    if (connection == nullptr) {
        return;
    }
    log("PRE ", event, *connection, "SERVERBOUND");
    receive(event, *connection);
    if (debug_.logsPostTransform()) {
        log("POST", event, *connection, "SERVERBOUND");
    }
}

void PacketListener::onPacketSend(endstone::PacketSendEvent &event)
{
    UserConnection *connection = touch(event);
    if (connection == nullptr) {
        return;
    }
    log("PRE ", event, *connection, "CLIENTBOUND");
    send(event, *connection);
    if (debug_.logsPostTransform()) {
        log("POST", event, *connection, "CLIENTBOUND");
    }
}

void PacketListener::onPlayerLogin(endstone::PlayerLoginEvent &event)
{
    endstone::Player &player = event.getPlayer();
    UserConnection *connection = connections_->get(player.getAddress());
    if (connection == nullptr) {
        return;
    }

    const ProtocolVersion announced = connection->getClientVersion();
    const ProtocolVersion dialect = ProtocolVersions::dialectOf(announced, player.getGameVersion());
    if (dialect == announced) {
        return;
    }

    connection->setClientVersion(dialect, server_version_);
    logger_->info("{} speaks {}, which shares protocol {} with older builds; translating as {}.",
                  player.getAddress().getHostname(), player.getGameVersion(),
                  ProtocolVersions::networkVersion(dialect), static_cast<int>(dialect));
}

void PacketListener::onPlayerQuit(endstone::PlayerQuitEvent &event)
{
    connections_->onDisconnect(event.getPlayer().getAddress());
}

} // namespace endweave
