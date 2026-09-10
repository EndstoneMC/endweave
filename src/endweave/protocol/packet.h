#pragma once

// The umbrella: `packet_of_t` is `void` for an id whose module isn't included, and two `void`s compare equal.
#include <bedrock/protocol.hpp>
#include <bedrock/protocol/packet.hpp>

namespace bp = bedrock::protocol;

namespace endweave {

using bp::has_packet_v;
using bp::packet_of_t;

} // namespace endweave
