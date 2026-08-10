#pragma once

#include "endweave/config.h"

#include <bedrock/enum.hpp>
#include <bedrock/protocol.hpp>
#include <cstddef>
#include <endstone/endstone.hpp>
#include <format>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace bp = bedrock::protocol;

namespace endweave {

/** A packet id as `NAME(id) (0xHH)`, or just the number where the schema has no name for
 * it -- most ids are unmodelled, and an unnamed one is exactly what is worth seeing.
 * @see ViaVersion ProtocolUtil. */
inline std::string packetLabel(int id)
{
    const std::string_view name = bp::enum_name(static_cast<bp::MinecraftPacketIds>(id));
    if (name.empty()) {
        return std::format("{} (0x{:02X})", id, id);
    }
    return std::format("{}({}) (0x{:02X})", name, id, id);
}

/** Logs every packet either side of its transform, so a connection that dies can be read
 * back packet by packet. Configured from `config.toml`, and off by default: one busy
 * connection is thousands of lines a second.
 * @see ViaVersion DebugHandler. */
class DebugHandler {
public:
    DebugHandler(endstone::Logger &logger, Config::Debug config)
        : logger_(&logger), enabled_(config.enabled), packets_(std::move(config.packets)),
          log_post_transform_(config.log_post_transform)
    {
    }

    [[nodiscard]] bool isEnabled() const
    {
        return enabled_;
    }

    /** Whether a packet is logged a second time once translated. */
    [[nodiscard]] bool logsPostTransform() const
    {
        return enabled_ && log_post_transform_;
    }

    /** Whether this id passes the filter. An empty filter passes every packet. */
    [[nodiscard]] bool shouldLog(int id) const
    {
        return enabled_ && (packets_.empty() || packets_.contains(id));
    }

    /** `PRE : <address> SERVERBOUND: START_GAME(11) (0x0B) [2168] 412b`
     * @see ViaVersion ProtocolPipelineImpl#logPacket. */
    void logPacket(std::string_view stage, std::string_view address, std::string_view direction, int id,
                   int client_version, std::size_t size) const
    {
        if (!shouldLog(id)) {
            return;
        }
        logger_->debug("{}: {} {}: {} [{}] {}b", stage, address, direction, packetLabel(id), client_version, size);
    }

private:
    endstone::Logger *logger_;
    bool enabled_;
    std::set<int> packets_;
    bool log_post_transform_;
};

} // namespace endweave
