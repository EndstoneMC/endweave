#pragma once

#include "endweave/connection/manager.h"
#include "endweave/protocol/direction.h"

#include <endstone/event/player/player_quit_event.h>
#include <endstone/event/server/packet_receive_event.h>
#include <endstone/event/server/packet_send_event.h>
#include <endstone/util/socket_address.h>
#include <string>

namespace endweave {

/**
 * Builds the key a connection is tracked under.
 *
 * The player object does not exist until well after the version handshake, so the peer address
 * is the only thing available to correlate on.
 *
 * @param address The peer address.
 * @return The key.
 */
std::string addressKey(const endstone::SocketAddress &address);

/**
 * Threads every packet through its connection's pipeline.
 *
 * @note Every handler runs on the server thread, in the tick, for every packet in both
 * directions.
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
    // PacketReceiveEvent and PacketSendEvent carry the same accessors but share no base that
    // declares them, so one template serves both.
    template <class Event>
    void translate(Direction direction, Event &event);

    ConnectionManager *connections_;
};

} // namespace endweave
