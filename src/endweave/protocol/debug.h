#pragma once

#include "endweave/config.h"

#include <bedrock/protocol.hpp>
#include <bedrock/protocol/enum.hpp>
#include <cstddef>
#include <endstone/endstone.hpp>
#include <format>
#include <iterator>
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

    /** `PRE : <address> SERVERBOUND: START_GAME(11) (0x0B) [2168] 412b`, and the body under it.
     * A translation that fails says only which packet and why, and the bytes are the rest of it.
     * @see ViaVersion ProtocolPipelineImpl#logPacket. */
    void logPacket(std::string_view stage, std::string_view address, std::string_view direction, int id,
                   int client_version, std::string_view payload) const
    {
        if (!shouldLog(id)) {
            return;
        }
        logger_->debug("{}: {} {}: {} [{}] {}b", stage, address, direction, packetLabel(id), client_version,
                       payload.size());
        logger_->debug("{}: {}", stage, hex(payload));
    }

private:
    /** The body as hex, capped: a filter narrow enough to want the bytes is narrow enough that
     * the first quarter-kilobyte of them is the interesting part. */
    static std::string hex(std::string_view payload)
    {
        constexpr std::size_t kMax = 256;
        const std::size_t shown = payload.size() < kMax ? payload.size() : kMax;
        std::string out;
        out.reserve(shown * 3 + 16);
        for (std::size_t i = 0; i < shown; ++i) {
            std::format_to(std::back_inserter(out), "{:02X} ", static_cast<unsigned char>(payload[i]));
        }
        if (shown < payload.size()) {
            std::format_to(std::back_inserter(out), "... +{}", payload.size() - shown);
        }
        return out;
    }

    endstone::Logger *logger_;
    bool enabled_;
    std::set<int> packets_;
    bool log_post_transform_;
};

} // namespace endweave
