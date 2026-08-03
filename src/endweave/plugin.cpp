#include "endweave/plugin.h"

#include "endweave/version.h"

#include <chrono>
#include <cstdint>
#include <endstone/endstone.hpp>

namespace endweave {
namespace {

constexpr std::chrono::seconds kIdleTimeout{120};
constexpr std::uint64_t kSweepPeriodTicks = 20 * 60;

} // namespace

void Plugin::onEnable()
{
    registerEvent(&PacketListener::onPacketReceive, listener_);
    registerEvent(&PacketListener::onPacketSend, listener_);
    registerEvent(&PacketListener::onPlayerQuit, listener_);

    getServer().getScheduler().runTaskTimer(
        *this,
        [this] {
            connections_.sweep(kIdleTimeout);
        },
        kSweepPeriodTicks, kSweepPeriodTicks);

    getLogger().info("Endweave enabled.");
}

void Plugin::onDisable()
{
    getLogger().info("Endweave disabled.");
}

} // namespace endweave

ENDSTONE_PLUGIN(/*name=*/"endweave", /*version=*/ENDWEAVE_VERSION, /*main_class=*/endweave::Plugin)
{
    prefix = "Endweave";
    description = "Bedrock protocol translation plugin for Endstone.";
    website = "https://github.com/EndstoneMC/endweave";
    authors = {"Vincent <magicdroidx@gmail.com>"};
}
