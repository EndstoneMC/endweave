#include "endweave/connection/connection.h"

#include "endweave/protocol/manager.h"

#include <utility>

namespace endweave {

UserConnection::UserConnection(ProtocolManager &protocol_manager, endstone::Logger &logger, std::string address,
                               int server_protocol_version)
    : protocol_manager_(&protocol_manager), logger_(&logger), address_(std::move(address)),
      protocol_info_(ProtocolPipeline{protocol_manager.getBaseProtocols()}, server_protocol_version),
      last_seen_(std::chrono::steady_clock::now())
{
}

void UserConnection::reportTranslationError(int packet_id, const std::error_code &error)
{
    const auto [it, inserted] = reported_errors_.try_emplace(packet_id, error);
    if (!inserted && it->second == error) {
        return;
    }
    it->second = error;
    logger_->error("Translation failed for packet {} from {}: {}", packet_id, address_, error.message());
}

} // namespace endweave
