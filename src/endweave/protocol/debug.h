#pragma once

#include <bedrock/enum.hpp>
#include <bedrock/protocol.hpp>
#include <cstddef>
#include <endstone/endstone.hpp>
#include <format>
#include <string>
#include <string_view>

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
 * back packet by packet. On by default while the version pair is being brought up: the
 * lines go to the debug channel, so a server at the default level still sees nothing.
 * @see ViaVersion DebugHandler. */
class DebugHandler {
public:
    explicit DebugHandler(endstone::Logger &logger, bool enabled = true) : logger_(&logger), enabled_(enabled) {}

    [[nodiscard]] bool isEnabled() const
    {
        return enabled_;
    }

    void setEnabled(bool enabled)
    {
        enabled_ = enabled;
    }

    /** `PRE : <address> SERVERBOUND: START_GAME(11) (0x0B) [2168] 412b`
     * @see ViaVersion ProtocolPipelineImpl#logPacket. */
    void logPacket(std::string_view stage, std::string_view address, std::string_view direction, int id,
                   int client_version, std::size_t size) const
    {
        if (!enabled_) {
            return;
        }
        logger_->debug("{}: {} {}: {} [{}] {}b", stage, address, direction, packetLabel(id), client_version, size);
    }

private:
    endstone::Logger *logger_;
    bool enabled_;
};

} // namespace endweave
