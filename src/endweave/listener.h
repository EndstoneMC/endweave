#pragma once

#include "endweave/connection/manager.h"
#include "endweave/protocol/direction.h"

#include <endstone/endstone.hpp>

namespace endweave {

/**
 * Threads every packet through its connection's pipeline.
 *
 * @note Runs on the server thread, in the tick, for every packet in both directions.
 * @note endweave-specific platform binding, the Endstone-event analogue of ViaVersion's netty
 * decode/encode handlers.
 */
class PacketListener {
public:
    /**
     * Constructs the listener.
     *
     * @param connections The connection table, which must outlive this listener.
     */
    explicit PacketListener(ConnectionManager &connections) : connections_(&connections) {}

    /**
     * Translates a packet arriving from a client into the server's version.
     *
     * @param event The receive event.
     */
    void onPacketReceive(endstone::PacketReceiveEvent &event);

    /**
     * Translates a packet leaving for a client into that client's version.
     *
     * @param event The send event.
     */
    void onPacketSend(endstone::PacketSendEvent &event);

    /**
     * Drops the connection state for a player who has left.
     *
     * @param event The quit event.
     */
    void onPlayerQuit(endstone::PlayerQuitEvent &event);

private:
    // One template serves PacketReceiveEvent and PacketSendEvent, which share no base.
    template <class Event>
    void translate(Direction direction, Event &event);

    ConnectionManager *connections_;
};

} // namespace endweave
