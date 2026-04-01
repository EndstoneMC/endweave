/// Base protocol implementation - version detection and login rewriting.

#include "endweave/protocol/base.h"

#include "endweave/codec/types/primitives.h"
#include "endweave/connection.h"
#include "endweave/protocol/packet_ids.h"

namespace endweave {

namespace {

void detect_client_protocol(PacketWrapper& wrapper) {
    auto& connection = wrapper.user();
    auto client_proto = wrapper.read<int_be>();
    if (client_proto) {
        connection.set_client_protocol(*client_proto);
    }
    wrapper.write<int_be>(connection.server_protocol());
}

void rewrite_login(PacketWrapper& wrapper) {
    auto& connection = wrapper.user();
    (void)wrapper.read<int_be>(); // Client Network Version
    wrapper.write<int_be>(connection.server_protocol());
}

} // namespace

std::unique_ptr<Protocol> create_base_protocol(int server_protocol) {
    auto p = std::make_unique<Protocol>(server_protocol, 0, "base", true);
    p->register_serverbound(static_cast<int>(PacketId::REQUEST_NETWORK_SETTINGS), detect_client_protocol);
    p->register_serverbound(static_cast<int>(PacketId::LOGIN), rewrite_login);
    return p;
}

} // namespace endweave
