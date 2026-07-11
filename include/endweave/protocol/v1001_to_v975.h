#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave::v1001_to_v975 {

// Downgrade boundary: server 1001 (newer), client 975 (older). Clientbound packets
// flow server->client = 1001->975, so they are downgraded.
Protocol create_protocol();

}  // namespace endweave::v1001_to_v975
