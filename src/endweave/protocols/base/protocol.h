#pragma once

#include "endweave/protocol/protocol.h"

namespace endweave {

/**
 * The always-on protocol at the head of every pipeline, mirroring ViaVersion's
 * InitialBaseProtocol.
 *
 * Does no version translation. It detects the client's version from the handshake, builds the
 * rest of the pipeline from it, rewrites the version the server is asked for, and logs packet
 * violations. Being present even on a same-version connection is what makes that last one work.
 */
class InitialBaseProtocol : public AbstractProtocol {
public:
    InitialBaseProtocol() : AbstractProtocol("base") {}

    [[nodiscard]] bool isBaseProtocol() const override
    {
        return true;
    }

protected:
    void registerPackets() override;
};

} // namespace endweave
