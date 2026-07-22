#include "endweave/protocols/base/protocol.h"

#include "endweave/connection/connection.h"
#include "endweave/protocol/manager.h"

#include <bedrock/stream.hpp>
#include <bit>
#include <cstdint>
#include <expected>
#include <string>
#include <system_error>

namespace endweave {
namespace {

namespace bp = bedrock::protocol;

/**
 * Records the client's version from the handshake and builds the pipeline from it -- the
 * Bedrock analogue of ViaVersion building it inside its own handshake handler.
 */
std::expected<PacketAction, std::error_code> detectClientVersion(UserConnection &connection, bp::BinaryReader &in,
                                                                 bp::BinaryWriter &out)
{
    const auto client_version = in.read<std::int32_t, std::endian::big>();
    if (!client_version) {
        return std::unexpected(client_version.error());
    }

    ProtocolInfo &info = connection.getProtocolInfo();
    info.setProtocolVersion(*client_version);
    const int server_version = info.getServerProtocolVersion();

    if (*client_version != server_version) {
        auto path = connection.getProtocolManager().getProtocolPath(*client_version, server_version);
        if (!path) {
            // Leave the client's own version on the wire so BDS rejects the login itself.
            connection.getLogger().warning("No protocol path from client {} to server {} for {}", *client_version,
                                           server_version, connection.getAddress());
            out.write<std::int32_t, std::endian::big>(*client_version);
            return PacketHandlers::passthrough()(connection, in, out);
        }
        info.getPipeline().add(*path);
        connection.getLogger().info("Translating {} for client {} (server {})", connection.getAddress(),
                                    *client_version, server_version);
    }

    out.write<std::int32_t, std::endian::big>(server_version);
    return PacketHandlers::passthrough()(connection, in, out);
}

/**
 * Rewrites the version the login claims, so BDS sees a server-version login.
 */
std::expected<PacketAction, std::error_code> rewriteLoginVersion(UserConnection &connection, bp::BinaryReader &in,
                                                                 bp::BinaryWriter &out)
{
    const auto client_version = in.read<std::int32_t, std::endian::big>();
    if (!client_version) {
        return std::unexpected(client_version.error());
    }

    ProtocolInfo &info = connection.getProtocolInfo();
    // Without a pipeline there is nothing to translate, so let the mismatch stand and BDS
    // reject the login rather than accepting a client we cannot serve.
    const bool translating = info.getPipeline().hasNonBaseProtocols();
    out.write<std::int32_t, std::endian::big>(translating ? info.getServerProtocolVersion() : *client_version);
    return PacketHandlers::passthrough()(connection, in, out);
}

/**
 * Logs a packet violation the server reported, then forwards the packet unchanged.
 */
std::expected<PacketAction, std::error_code> logPacketViolation(UserConnection &connection, bp::BinaryReader &in,
                                                                bp::BinaryWriter &out)
{
    const std::string_view body = in.getView().substr(in.getReadPointer());
    out.writeRawBytes(body);

    bp::BinaryReader reader{body};
    const auto type = reader.readVarInt<std::int32_t>();
    const auto severity = reader.readVarInt<std::int32_t>();
    const auto packet_id = reader.readVarInt<std::int32_t>();
    const auto context = reader.read<std::string>();
    if (type && severity && packet_id && context) {
        connection.getLogger().warning("Packet violation from {}: type={} severity={} packet={} context={}",
                                       connection.getAddress(), *type, *severity, *packet_id, *context);
    }
    else {
        connection.getLogger().warning("Packet violation from {} (undecodable)", connection.getAddress());
    }
    return PacketAction::Translated;
}

} // namespace

void InitialBaseProtocol::registerPackets()
{
    registerServerbound(MinecraftPacketIds::RequestNetworkSettings, detectClientVersion);
    registerServerbound(MinecraftPacketIds::Login, rewriteLoginVersion);
    registerClientbound(MinecraftPacketIds::PacketViolationWarning, logPacketViolation);
}

} // namespace endweave
