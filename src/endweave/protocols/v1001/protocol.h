#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave {

/**
 * Protocol 1001 (1.26.30). Owns both directions of the wire diff with the version before it.
 * Carries no handlers yet. Converters will live in registerPackets() once translation is built.
 *
 * @see A ViaVersion forward protocol (e.g. Protocol1_20To1_20_2) fused with its ViaBackwards
 * backward protocol (Protocol1_20_2To1_20).
 */
template <>
class Protocol<ProtocolVersion::V1001> : public AbstractProtocol {
public:
    Protocol() : AbstractProtocol(ProtocolVersion::V1001) {}
};

} // namespace endweave
