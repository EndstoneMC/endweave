#pragma once

#include "endweave/connection/manager.h"
#include "endweave/listener.h"
#include "endweave/protocol/manager.h"

#include <endstone/plugin/plugin.h>
#include <optional>

namespace endweave {

/**
 * The Endstone plugin: owns the registry and the connection table, and wires the
 * listener up.
 */
class EndweavePlugin : public endstone::Plugin {
public:
    void onEnable() override;
    void onDisable() override;

private:
    ProtocolManager protocol_manager_;
    std::optional<ConnectionManager> connections_;
    std::optional<PacketListener> listener_;
};

} // namespace endweave
