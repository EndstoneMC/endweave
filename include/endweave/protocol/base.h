#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave {

// The always-on base protocol (client_protocol 0, is_base) that sits at the front
// of every pipeline: it detects the client version from the handshake, rewrites
// the login version, and logs packet violations. Mirrors ViaVersion's base.py.
Protocol create_base_protocol(int server_protocol);

} // namespace endweave
