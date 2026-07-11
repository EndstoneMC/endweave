#pragma once

namespace endweave {

// Transport direction of a packet, matching ViaVersion's Direction. Serverbound
// is client->server, clientbound is server->client; a directional protocol picks
// its handler table by this axis.
enum class Direction { Serverbound, Clientbound };

}  // namespace endweave
