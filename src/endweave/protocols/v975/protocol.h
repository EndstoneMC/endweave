#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave {

/**
 * Protocol 975 (1.26.20), the lowest version endweave models.
 *
 * The root of the chain, so nothing is registered below it and it carries no handlers. It is
 * still a real node: every path that reaches 975 ends here.
 */
template <>
class Protocol<ProtocolVersion::V975> : public AbstractProtocol {
public:
    Protocol() : AbstractProtocol(ProtocolVersion::V975) {}
};

} // namespace endweave
