#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave {

/**
 * Protocol 1001 (1.26.30).
 *
 * Owns both directions of the wire diff between itself and the version registered before it.
 * Carries no handlers, so every packet passes through untouched: the converters live in
 * registerPackets(), which is empty until the translation layer is redesigned.
 */
template <>
class Protocol<ProtocolVersion::V1001> : public AbstractProtocol {
public:
    Protocol() : AbstractProtocol(ProtocolVersion::V1001) {}
};

} // namespace endweave
