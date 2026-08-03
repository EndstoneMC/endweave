#pragma once

#include "endweave/protocol/version_node.h"

namespace endweave {

/**
 * Protocol 975 (1.26.20), the lowest version endweave models and the root of the chain. The root
 * has no predecessor, so its edge is empty and it declares nothing, but every path that reaches
 * 975 ends here.
 *
 * @see ViaVersion's lowest supported protocol (e.g. Protocol1_8To1_9).
 */
template <>
class Protocol<ProtocolVersion::v26_20> final
    : public VersionNode<ProtocolVersion::v26_20, Protocol<ProtocolVersion::v26_20>> {};

} // namespace endweave
