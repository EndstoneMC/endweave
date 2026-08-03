#pragma once

#include "endweave/connection/manager.h"

#include <endstone/endstone.hpp>

namespace endweave {

class PacketListener {
public:
    explicit PacketListener(ConnectionManager &connections) : connections_(&connections) {}

    void onPacketReceive(endstone::PacketReceiveEvent &event);

    void onPacketSend(endstone::PacketSendEvent &event);

    void onPlayerQuit(endstone::PlayerQuitEvent &event);

private:
    template <class Event>
    void handle(Event &event);

    ConnectionManager *connections_;
};

} // namespace endweave
