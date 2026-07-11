#include "endweave/protocol/base.h"

#include "endweave/connection.h"
#include "endweave/protocol/packet_ids.h"

#include <bedrock/stream.hpp>
#include <bit>
#include <cstdint>
#include <expected>
#include <string>
#include <system_error>

namespace endweave {
namespace {

namespace bp = bedrock::protocol;

// The C++ analogue of PacketWrapper.to_bytes()'s trailing copy: forward every
// still-unread input byte to the output.
void copy_remaining(bp::BinaryReader &in, bp::BinaryWriter &out)
{
    out.writeRawBytes(in.getView().substr(in.getReadPointer()));
}

// RequestNetworkSettings: read the client's protocol (big-endian int32), record it
// on the connection, and rewrite the field to the server's version so downstream
// stages see a server-version packet.
std::expected<void, std::error_code> detect_client_protocol(UserConnection &connection, bp::BinaryReader &in,
                                                            bp::BinaryWriter &out)
{
    auto client = in.read<std::int32_t, std::endian::big>();
    if (!client) {
        return std::unexpected(client.error());
    }
    connection.set_client_protocol(*client);
    out.write<std::int32_t, std::endian::big>(connection.server_protocol());
    copy_remaining(in, out);
    connection.log().debug("Client connected with protocol " + std::to_string(*client) + " (server " +
                           std::to_string(connection.server_protocol()) + ")");
    return {};
}

// Login: drop the client's version and emit the server's, then copy the rest.
std::expected<void, std::error_code> rewrite_login(UserConnection &connection, bp::BinaryReader &in,
                                                   bp::BinaryWriter &out)
{
    auto client = in.read<std::int32_t, std::endian::big>();
    if (!client) {
        return std::unexpected(client.error());
    }
    out.write<std::int32_t, std::endian::big>(connection.server_protocol());
    copy_remaining(in, out);
    return {};
}

// PacketViolationWarning: byte-identical passthrough; decode the fields only to log.
std::expected<void, std::error_code> log_packet_violation(UserConnection &connection, bp::BinaryReader &in,
                                                          bp::BinaryWriter &out)
{
    out.writeRawBytes(in.getView());
    bp::BinaryReader reader{in.getView()};
    auto type = reader.readVarInt<std::int32_t>();
    auto severity = reader.readVarInt<std::int32_t>();
    auto packet_id = reader.readVarInt<std::int32_t>();
    auto context = reader.read<std::string>();
    if (type && severity && packet_id && context) {
        connection.log().warning("Server reported packet violation: type=" + std::to_string(*type) +
                                 " severity=" + std::to_string(*severity) + " packet=" + std::to_string(*packet_id) +
                                 " context=" + *context);
    }
    else {
        connection.log().warning("Server reported a packet violation (undecodable)");
    }
    return {};
}

} // namespace

Protocol create_base_protocol(int server_protocol)
{
    Protocol protocol{server_protocol, 0, "base", true};
    protocol.register_serverbound(static_cast<int>(PacketId::RequestNetworkSettings), detect_client_protocol);
    protocol.register_serverbound(static_cast<int>(PacketId::Login), rewrite_login);
    protocol.register_clientbound(static_cast<int>(PacketId::PacketViolationWarning), log_packet_violation);
    return protocol;
}

} // namespace endweave
