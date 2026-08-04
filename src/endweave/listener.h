#pragma once

#include "endweave/connection/manager.h"
#include "endweave/protocol/debug.h"
#include "endweave/protocol/handler.h"

#include <endstone/endstone.hpp>
#include <string_view>

namespace endweave {

class PacketListener {
public:
    PacketListener(ConnectionManager &connections, endstone::Logger &logger)
        : connections_(&connections), logger_(&logger), debug_(logger)
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

    template <class Event>
    void log(std::string_view stage, Event &event, const UserConnection &connection, std::string_view direction) const;

    void receive(endstone::PacketReceiveEvent &event, UserConnection &connection);

    void send(endstone::PacketSendEvent &event, UserConnection &connection);

    ConnectionManager *connections_;
    endstone::Logger *logger_;
    DebugHandler debug_;
};

} // namespace endweave
