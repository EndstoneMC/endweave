#pragma once

#include "endweave/connection/manager.h"
#include "endweave/listener.h"

#include <endstone/endstone.hpp>
#include <optional>

namespace endweave {

class Plugin : public endstone::Plugin {
public:
    void onEnable() override;

    void onDisable() override;

private:
    ConnectionManager connections_;
    // The loader binds the plugin's logger after construction, so the listener cannot be
    // built until onEnable.
    std::optional<PacketListener> listener_;
};

} // namespace endweave
