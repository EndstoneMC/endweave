#pragma once

#include "endweave/connection/manager.h"
#include "endweave/protocol/handler.h"

#include <endstone/endstone.hpp>

namespace endweave {

class PacketListener {
public:
    PacketListener(ConnectionManager &connections, endstone::Logger &logger)
        : connections_(&connections), logger_(&logger)
    {
    }

    void onPacketReceive(endstone::PacketReceiveEvent &event);

    void onPacketSend(endstone::PacketSendEvent &event);

    void onPlayerQuit(endstone::PlayerQuitEvent &event);

private:
    /** The connection the event belongs to, or null once it has been torn down. */
    template <class Event>
    UserConnection *touch(Event &event);

    /** Rewrites the payload where the two versions disagree on the packet's shape, and
     * cancels it where the destination has no such packet at all. */
    template <class Event>
    void translate(Event &event, const PacketHandlers &handlers);

    ConnectionManager *connections_;
    endstone::Logger *logger_;
};

} // namespace endweave
