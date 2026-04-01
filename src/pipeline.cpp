/// ProtocolPipeline implementation.

#include "endweave/pipeline.h"

#include <algorithm>

#include "endweave/protocol/packet_ids.h"

namespace endweave {

bool ProtocolPipeline::on_packet_receive(const std::string& address, int packet_id,
                                          std::string_view payload, std::string& output,
                                          bool& cancelled) {
    cancelled = false;
    auto& connection = connections_.get_or_create(address);
    auto& pipeline = get_pipeline(connection);

    auto current_payload = payload;
    bool modified = false;

    for (auto* protocol : pipeline) {
        if (!protocol->has_handler_or_cancel(Direction::SERVERBOUND, packet_id))
            continue;

        PacketWrapper wrapper(current_payload, &connection);
        protocol->transform(Direction::SERVERBOUND, packet_id, wrapper);

        if (wrapper.cancelled()) {
            cancelled = true;
            return false;
        }
        output = wrapper.to_string();
        current_payload = output;
        modified = true;
    }

    return modified;
}

bool ProtocolPipeline::on_packet_send(const std::string& address, int packet_id,
                                       std::string_view payload, std::string& output,
                                       bool& cancelled) {
    cancelled = false;
    auto* connection = connections_.get(address);
    if (!connection || !connection->protocol_pipeline())
        return false;

    auto* cb_pipeline = connection->clientbound_pipeline();
    if (!cb_pipeline) return false;

    auto current_payload = payload;
    bool modified = false;

    for (auto* protocol : *cb_pipeline) {
        if (!protocol->has_handler_or_cancel(Direction::CLIENTBOUND, packet_id))
            continue;

        PacketWrapper wrapper(current_payload, connection);
        protocol->transform(Direction::CLIENTBOUND, packet_id, wrapper);

        if (wrapper.cancelled()) {
            cancelled = true;
            return false;
        }
        output = wrapper.to_string();
        current_payload = output;
        modified = true;
    }

    return modified;
}

const std::vector<Protocol*>& ProtocolPipeline::get_pipeline(UserConnection& connection) {
    if (auto* cached = connection.protocol_pipeline())
        return *cached;

    auto& base = manager_.base_protocols();

    if (connection.client_protocol() == 0) {
        // Pre-handshake: return base-only without caching
        return base;
    }

    if (!connection.needs_translation()) {
        connection.set_pipeline(base);
        return *connection.protocol_pipeline();
    }

    auto* chain = manager_.get_path(connection.server_protocol(), connection.client_protocol());
    if (!chain) {
        if (!connection.warned_no_chain()) {
            connection.set_warned_no_chain(true);
        }
        connection.set_pipeline(base);
        return *connection.protocol_pipeline();
    }

    for (auto* protocol : *chain)
        protocol->init(connection);

    auto pipeline = base;
    pipeline.insert(pipeline.end(), chain->begin(), chain->end());
    connection.set_pipeline(pipeline);

    // Pre-compute clientbound order: base first, then chain reversed
    auto cb_pipeline = base;
    cb_pipeline.insert(cb_pipeline.end(), chain->rbegin(), chain->rend());
    connection.set_clientbound_pipeline(cb_pipeline);

    return *connection.protocol_pipeline();
}

} // namespace endweave
