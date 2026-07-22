#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave {

/**
 * Protocol 1001 (1.26.30).
 *
 * Owns both directions of the wire diff between itself and the version registered before it:
 * the upgrade into 1001 and the downgrade out of it. Which version that is comes from the
 * registration order in ProtocolManager::registerProtocols(), not from this class.
 */
template <>
class Protocol<ProtocolVersion::V1001> : public AbstractProtocol {
public:
    Protocol() : AbstractProtocol(ProtocolVersion::V1001) {}

protected:
    void registerPackets() override;
};

} // namespace endweave
