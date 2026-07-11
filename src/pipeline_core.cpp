#include "endweave/pipeline_core.h"

#include "endweave/connection.h"
#include "endweave/protocol/manager.h"
#include "endweave/protocol/protocol.h"

#include <bedrock/stream.hpp>
#include <optional>
#include <string>
#include <utility>

namespace endweave {

PipelineRunResult run_pipeline(std::span<const Protocol *const> pipeline, Direction direction, int packet_id,
                               UserConnection &connection, std::string_view payload)
{
    std::string current(payload);
    bool changed = false;
    for (const Protocol *protocol : pipeline) {
        if (!protocol->has_handler_or_cancel(direction, packet_id)) {
            continue;
        }
        bedrock::protocol::BinaryReader in{current};
        std::string buffer;
        bedrock::protocol::BinaryWriter out{buffer};
        auto result = protocol->transform(direction, packet_id, connection, in, out);
        if (!result) {
            return {PipelineRunResult::Status::Failed, {}, protocol, result.error()};
        }
        switch (*result) {
        case TransformResult::Cancelled:
            return {PipelineRunResult::Status::Cancelled, {}, nullptr, {}};
        case TransformResult::Translated:
            current = std::move(buffer);
            changed = true;
            break;
        case TransformResult::Passthrough:
            break; // unreachable while has_handler_or_cancel is true
        }
    }
    const auto status = changed ? PipelineRunResult::Status::Rewritten : PipelineRunResult::Status::Unchanged;
    return {status, std::move(current), nullptr, {}};
}

std::vector<const Protocol *> resolve_pipeline(ProtocolManager &manager, UserConnection &connection)
{
    if (connection.protocol_pipeline().has_value()) {
        return *connection.protocol_pipeline();
    }

    std::vector<const Protocol *> base = manager.base_protocols();

    if (connection.client_protocol() == 0) {
        return base; // pre-handshake: base-only, NOT cached (re-evaluated until detected)
    }

    if (!connection.needs_translation()) {
        connection.set_pipelines(base, std::nullopt);
        return base;
    }

    auto chain = manager.get_path(connection.server_protocol(), connection.client_protocol());
    if (!chain) {
        if (!connection.warned_no_chain()) {
            connection.set_warned_no_chain(true);
            connection.log().warning("No protocol chain for server=" + std::to_string(connection.server_protocol()) +
                                     " client=" + std::to_string(connection.client_protocol()) + " from " +
                                     connection.address());
        }
        connection.set_pipelines(base, std::nullopt);
        return base;
    }

    for (const Protocol *protocol : *chain) {
        protocol->init(connection);
    }

    std::vector<const Protocol *> serverbound = base;
    serverbound.insert(serverbound.end(), chain->begin(), chain->end());

    std::vector<const Protocol *> clientbound = base;
    clientbound.insert(clientbound.end(), chain->rbegin(), chain->rend());

    connection.set_pipelines(serverbound, clientbound);
    return serverbound;
}

} // namespace endweave
