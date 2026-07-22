#include "endweave/plugin.h"

#include "endweave/version.h"

#include <chrono>
#include <cstdint>
#include <endstone/scheduler/scheduler.h>
#include <string>

namespace endweave {
namespace {

// A connection that fails before login never produces a quit event, so the table is swept as
// well as evicted on disconnect.
constexpr std::chrono::seconds kIdleTimeout{120};
constexpr std::uint64_t kSweepPeriodTicks = 20 * 60;

} // namespace

void EndweavePlugin::onEnable()
{
    const int server_protocol_version = getServer().getProtocolVersion();

    protocol_manager_.registerProtocols();
    protocol_manager_.refreshVersions(server_protocol_version);

    connections_.emplace(protocol_manager_, getLogger(), server_protocol_version);
    listener_.emplace(*connections_);

    // Lowest serverbound so an old client's packets reach the server version before any other
    // plugin decodes them; Highest clientbound so downgrades run after every other plugin has
    // written. Not Monitor -- that must not mutate.
    registerEvent(&PacketListener::onPacketReceive, *listener_, endstone::EventPriority::Lowest,
                  /*ignore_cancelled=*/true);
    registerEvent(&PacketListener::onPacketSend, *listener_, endstone::EventPriority::Highest,
                  /*ignore_cancelled=*/true);
    registerEvent(&PacketListener::onPlayerQuit, *listener_);

    getServer().getScheduler().runTaskTimer(
        *this,
        [this] {
            connections_->sweep(kIdleTimeout);
        },
        kSweepPeriodTicks, kSweepPeriodTicks);

    std::string versions;
    for (const int version : protocol_manager_.getSupportedVersions()) {
        versions += (versions.empty() ? "" : ", ") + std::to_string(version);
    }
    getLogger().info("Endweave enabled. Server protocol {}, serving {}.", server_protocol_version, versions);
}

void EndweavePlugin::onDisable()
{
    getLogger().info("Endweave disabled.");
}

} // namespace endweave

ENDSTONE_PLUGIN(/*name=*/"endweave", /*version=*/ENDWEAVE_VERSION, /*main_class=*/endweave::EndweavePlugin)
{
    prefix = "Endweave";
    description = "Bedrock protocol translation plugin for Endstone.";
    website = "https://github.com/EndstoneMC/endweave";
    authors = {"Vincent <magicdroidx@gmail.com>"};
}
