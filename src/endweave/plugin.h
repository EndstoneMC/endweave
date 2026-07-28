#pragma once

#include "endweave/connection/manager.h"
#include "endweave/listener.h"
#include "endweave/protocol/manager.h"

#include <endstone/endstone.hpp>
#include <memory>

namespace endweave {

/**
 * The Endstone plugin: owns the registry and the connection table, and wires the listener up.
 *
 * @note endweave-specific platform binding, standing in for a ViaVersion platform module and its
 * ViaManager. Owns what ViaVersion reaches through the Via global.
 */
class Plugin : public endstone::Plugin {
public:
    void onEnable() override;
    void onDisable() override;

private:
    ProtocolManager protocol_manager_;                 // ViaVersion: owned by ViaManager
    std::unique_ptr<ConnectionManager> connections_;   // ViaVersion: owned by ViaManager
    std::unique_ptr<PacketListener> listener_;         // ViaVersion: the netty decode/encode handlers
};

} // namespace endweave
