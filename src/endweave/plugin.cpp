#include "endweave/plugin.h"

#include "endstone/logger.h"
#include "endweave/config.h"
#include "endweave/version.h"

#include <chrono>
#include <endstone/endstone.hpp>

namespace endweave {

void Plugin::onEnable()
{
    const Config config = Config::load(getDataFolder(), getLogger());
    getLogger().setLevel(config.debug.enabled ? endstone::Logger::Debug : endstone::Logger::Info);

    const int protocol = getServer().getProtocolVersion();
    // 1.26.40 and 1.26.44 both report 2168, so the server's own dialect needs the game version too.
    const ProtocolVersion server_version =
        ProtocolVersions::dialectOf(ProtocolVersions::getProtocolVersion(protocol), getServer().getMinecraftVersion());
    if (server_version == ProtocolVersion::UNKNOWN) {
        getLogger().error("This server speaks protocol {}, which endweave does not translate. Standing down.",
                          protocol);
        return;
    }

    PacketListener &listener = listener_.emplace(connections_, getLogger(), config.debug, server_version);
    registerEvent(&PacketListener::onPacketReceive, listener);
    registerEvent(&PacketListener::onPacketSend, listener);
    registerEvent(&PacketListener::onPlayerLogin, listener);
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
