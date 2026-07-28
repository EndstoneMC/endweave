#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave {

/**
 * Protocol 975 (1.26.20), the lowest version endweave models and the root of the chain. Carries
 * no handlers, but every path that reaches 975 ends here.
 *
 * @see ViaVersion's lowest supported protocol (e.g. Protocol1_8To1_9).
 */
template <>
class Protocol<ProtocolVersion::v26_20> : public AbstractProtocol {
public:
    Protocol() : AbstractProtocol(ProtocolVersion::v26_20) {}
};

} // namespace endweave
