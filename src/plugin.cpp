#include "endweave/plugin.h"

#include "endweave/pipeline_core.h"
#include "endweave/protocol/base.h"
#include "endweave/protocol/v1001_to_v975.h"
#include "endweave/protocol/v975_to_v1001.h"
#include "endweave/version.h"

#include <string>

namespace endweave {
namespace {

std::string addr_key(const endstone::SocketAddress &address)
{
    return address.getHostname() + ":" + std::to_string(address.getPort());
}

} // namespace

void EndweavePlugin::onEnable()
{
    const int server_protocol = getServer().getProtocolVersion();

    sink_.emplace(getLogger());
    base_.emplace(create_base_protocol(server_protocol));
    up_.emplace(v975_to_v1001::create_protocol());
    down_.emplace(v1001_to_v975::create_protocol());

    manager_.register_base(*base_);
    manager_.register_protocol(*up_);
    manager_.register_protocol(*down_);
    connections_.emplace(server_protocol, *sink_);

    registerEvent(&EndweavePlugin::onPacketReceive, *this);
    registerEvent(&EndweavePlugin::onPacketSend, *this);
    registerEvent(&EndweavePlugin::onPlayerQuit, *this);

    getLogger().info("Endweave enabled (server protocol {}).", server_protocol);
}

void EndweavePlugin::onDisable()
{
    getLogger().info("Endweave disabled.");
}

void EndweavePlugin::onPacketReceive(endstone::PacketReceiveEvent &event)
{
    UserConnection &connection = connections_->get_or_create(addr_key(event.getAddress()));
    auto pipeline = resolve_pipeline(manager_, connection);
    auto result = run_pipeline(pipeline, Direction::Serverbound, event.getPacketId(), connection, event.getPayload());
    switch (result.status) {
    case PipelineRunResult::Status::Failed:
        getLogger().error("Serverbound translation failed for packet {}: {}", event.getPacketId(),
                          result.error.message());
        event.cancel();
        break;
    case PipelineRunResult::Status::Cancelled:
        event.cancel();
        break;
    case PipelineRunResult::Status::Rewritten:
        event.setPayload(result.payload);
        break;
    case PipelineRunResult::Status::Unchanged:
        break;
    }
}

void EndweavePlugin::onPacketSend(endstone::PacketSendEvent &event)
{
    UserConnection *connection = connections_->get(addr_key(event.getAddress()));
    if (connection == nullptr || !connection->clientbound_pipeline().has_value()) {
        return; // pre-handshake, same-version, or no chain -- nothing to translate clientbound
    }
    auto result = run_pipeline(*connection->clientbound_pipeline(), Direction::Clientbound, event.getPacketId(),
                               *connection, event.getPayload());
    switch (result.status) {
    case PipelineRunResult::Status::Failed:
        getLogger().error("Clientbound translation failed for packet {}: {}", event.getPacketId(),
                          result.error.message());
        event.cancel();
        break;
    case PipelineRunResult::Status::Cancelled:
        event.cancel();
        break;
    case PipelineRunResult::Status::Rewritten:
        event.setPayload(result.payload);
        break;
    case PipelineRunResult::Status::Unchanged:
        break;
    }
}

void EndweavePlugin::onPlayerQuit(endstone::PlayerQuitEvent &event)
{
    connections_->remove_by_address(addr_key(event.getPlayer().getAddress()));
}

} // namespace endweave

ENDSTONE_PLUGIN(/*name=*/"endweave", /*version=*/ENDWEAVE_VERSION, /*main_class=*/endweave::EndweavePlugin)
{
    prefix = "Endweave";
    description = "Bedrock protocol translation plugin for Endstone.";
    website = "https://github.com/EndstoneMC/endweave";
    authors = {"Vincent <magicdroidx@gmail.com>"};
}
