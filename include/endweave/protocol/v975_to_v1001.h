#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave::v975_to_v1001 {

// Upgrade boundary: server 975 (older), client 1001 (newer). Clientbound packets
// flow server->client = 975->1001, so they are upgraded.
Protocol create_protocol();

}  // namespace endweave::v975_to_v1001
