#pragma once

#include "endweave/connection/manager.h"
#include "endweave/listener.h"

#include <endstone/endstone.hpp>

namespace endweave {

class Plugin : public endstone::Plugin {
public:
    void onEnable() override;

    void onDisable() override;

private:
    ConnectionManager connections_;
    PacketListener listener_{connections_};
};

} // namespace endweave
