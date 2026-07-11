#pragma once

#include "endweave/connection.h"
#include "endweave/log_sink.h"
#include "endweave/protocol/manager.h"
#include "endweave/protocol/protocol.h"

#include <endstone/endstone.hpp>
#include <endstone/event/player/player_quit_event.h>
#include <endstone/event/server/packet_receive_event.h>
#include <endstone/event/server/packet_send_event.h>
#include <optional>
#include <string_view>

namespace endweave {

// Routes the core's LogSink seam to endstone::Logger.
class EndstoneLogSink : public LogSink {
public:
    explicit EndstoneLogSink(endstone::Logger &logger) : logger_(&logger) {}
    void debug(std::string_view message) override
    {
        logger_->debug(message);
    }
    void warning(std::string_view message) override
    {
        logger_->warning(message);
    }
    void error(std::string_view message) override
    {
        logger_->error(message);
    }

private:
    endstone::Logger *logger_;
};

// Endstone plugin entrypoint: intercepts packets both directions, threads each
// through the translation pipeline for its connection, and rewrites/cancels.
class EndweavePlugin : public endstone::Plugin {
public:
    void onEnable() override;
    void onDisable() override;

    void onPacketReceive(endstone::PacketReceiveEvent &event);
    void onPacketSend(endstone::PacketSendEvent &event);
    void onPlayerQuit(endstone::PlayerQuitEvent &event);

private:
    std::optional<EndstoneLogSink> sink_;
    std::optional<Protocol> base_;
    std::optional<Protocol> up_;
    std::optional<Protocol> down_;
    ProtocolManager manager_;
    std::optional<ConnectionManager> connections_;
};

} // namespace endweave
