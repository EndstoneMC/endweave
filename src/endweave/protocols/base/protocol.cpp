#include "endweave/protocols/base/protocol.h"

#include "endweave/connection/connection.h"
#include "endweave/protocol/error.h"
#include "endweave/protocol/manager.h"

#include <bedrock/protocol.hpp>
#include <expected>

namespace endweave {
namespace {

namespace bp = bedrock::protocol;

/** Records the client's version from the handshake and builds the pipeline. */
std::expected<bp::RequestNetworkSettingsPacket, PacketError> detectClientVersion(
    UserConnection &connection, const bp::RequestNetworkSettingsPacket &packet)
{
    const int client_version = packet.client_network_version;
    ProtocolInfo &info = connection.getProtocolInfo();
    info.setProtocolVersion(client_version);
    const int server_version = info.getServerProtocolVersion();
    if (client_version == server_version) {
        return packet;
    }

    auto path = connection.getProtocolManager().getProtocolPath(client_version, server_version);
    if (!path) {
        // Leave the client's version on the wire so BDS rejects the login.
        connection.getLogger().warning("No protocol path from client {} to server {} for {}", client_version,
                                       server_version, connection.getAddress());
        return packet;
    }
    info.getPipeline().add(path.value());
    connection.getLogger().info("Translating {} for client {} (server {})", connection.getAddress(), client_version,
                                server_version);

    bp::RequestNetworkSettingsPacket out = packet;
    out.client_network_version = server_version;
    return out;
}

/**
 * Rewrites the version the login claims, so BDS sees a server-version login.
 */
std::expected<bp::LoginPacket, PacketError> rewriteLoginVersion(UserConnection &connection,
                                                                const bp::LoginPacket &packet)
{
    ProtocolInfo &info = connection.getProtocolInfo();
    // No pipeline means nothing to translate, so let BDS reject the mismatch.
    if (!info.getPipeline().hasNonBaseProtocols()) {
        return packet;
    }

    bp::LoginPacket out = packet;
    out.client_network_version = info.getServerProtocolVersion();
    return out;
}

/**
 * Logs a packet violation the server reported, then forwards the packet unchanged.
 */
std::expected<bp::PacketViolationWarningPacket, PacketError> logPacketViolation(
    UserConnection &connection, const bp::PacketViolationWarningPacket &packet)
{
    connection.getLogger().warning("Packet violation from {}: type={} severity={} packet={} context={}",
                                   connection.getAddress(), static_cast<int>(packet.violation_type),
                                   static_cast<int>(packet.violation_severity),
                                   static_cast<int>(packet.violating_packet_id), packet.violation_context);
    return packet;
}

} // namespace

void InitialBaseProtocol::registerPackets()
{
    registerServerbound(&detectClientVersion);
    registerServerbound(&rewriteLoginVersion);
    registerClientbound(&logPacketViolation);
}

} // namespace endweave
