#include "endweave/plugin.h"

#include "endweave/version.h"

#include <chrono>
#include <cstdint>
#include <endstone/endstone.hpp>

namespace endweave {

void Plugin::onEnable()
{
    // The debug handler is on while this version pair is being brought up, and its lines
    // go to the debug channel, which the default level discards.
    getLogger().setLevel(endstone::Logger::Debug);

    PacketListener &listener = listener_.emplace(connections_, getLogger());
    registerEvent(&PacketListener::onPacketReceive, listener);
    registerEvent(&PacketListener::onPacketSend, listener);
    registerEvent(&PacketListener::onPlayerQuit, listener);

    getServer().getScheduler().runTaskTimer(
        *this,
        [this] {
            connections_.sweep(std::chrono::seconds(10));
        },
        20, 20);

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
