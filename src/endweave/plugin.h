#pragma once

#include "endweave/connection/manager.h"
#include "endweave/listener.h"
#include "endweave/protocol/manager.h"

#include <endstone/plugin/plugin.h>
#include <optional>

namespace endweave {

/**
 * The Endstone plugin: owns the registry and the connection table, and wires the listener up.
 *
 * @note endweave-specific platform binding, standing in for a ViaVersion platform module and its
 * ViaManager. Owns what ViaVersion reaches through the Via global.
 */
class EndweavePlugin : public endstone::Plugin {
public:
    void onEnable() override;
    void onDisable() override;

private:
    ProtocolManager protocol_manager_;              // ViaVersion: owned by ViaManager
    std::optional<ConnectionManager> connections_;  // ViaVersion: owned by ViaManager
    std::optional<PacketListener> listener_;        // ViaVersion: the netty decode/encode handlers
};

} // namespace endweave
