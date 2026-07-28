#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave {

/**
 * The always-on protocol at the head of every pipeline. Does no version translation: it detects
 * the client's version from the handshake, builds the rest of the pipeline, rewrites the version
 * the server is asked for, and logs packet violations.
 *
 * @see ViaVersion InitialBaseProtocol. The Bedrock handshake is RequestNetworkSettings rather
 * than CLIENT_INTENTION.
 */
class InitialBaseProtocol : public AbstractProtocol {
public:
    InitialBaseProtocol() : AbstractProtocol("base") {}

    /** @see ViaVersion InitialBaseProtocol#isBaseProtocol. */
    [[nodiscard]] bool isBaseProtocol() const override
    {
        return true;
    }

protected:
    /** @see ViaVersion InitialBaseProtocol#registerPackets. */
    void registerPackets() override;
};

} // namespace endweave
