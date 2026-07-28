#pragma once

#include "endweave/protocol/direction.h"

#include <cstddef>
#include <functional>
#include <vector>

namespace endweave {

class AbstractProtocol;

/**
 * One hop of a resolved path. The protocol is the newer of the hop's two nodes. Step picks which
 * table this traversal uses. Carries the version the stream is in after it.
 *
 * @see ViaVersion ProtocolPathEntry (api) and ProtocolPathEntryImpl (common). The step field is
 * endweave-specific: a node serves both directions.
 */
struct ProtocolPathEntry {
    const AbstractProtocol *protocol = nullptr; // ViaVersion: protocol()
    Step step = Step::Upgrade;                  // endweave-specific
    int output_protocol_version = 0;            // ViaVersion: outputProtocolVersion()

    friend bool operator==(const ProtocolPathEntry &, const ProtocolPathEntry &) = default;
};

/**
 * A resolved path, in serverbound order: client version first, server version last.
 *
 * @see ViaVersion List<ProtocolPathEntry>.
 */
using ProtocolPath = std::vector<ProtocolPathEntry>;

/**
 * The (client version, server version) pair a cached path is keyed by.
 *
 * @see ViaVersion ProtocolPathKey (api) and ProtocolPathKeyImpl (common).
 */
struct ProtocolPathKey {
    int client_protocol_version = 0; // ViaVersion: clientVersion
    int server_protocol_version = 0; // ViaVersion: serverVersion

    friend bool operator==(const ProtocolPathKey &, const ProtocolPathKey &) = default;
};

/**
 * Hashes a ProtocolPathKey, for the path cache.
 *
 * @note endweave-specific: ViaVersion's record derives its own hashCode.
 */
struct ProtocolPathKeyHash {
    std::size_t operator()(const ProtocolPathKey &key) const noexcept
    {
        const auto client = static_cast<unsigned long long>(static_cast<unsigned>(key.client_protocol_version));
        const auto server = static_cast<unsigned long long>(static_cast<unsigned>(key.server_protocol_version));
        return std::hash<unsigned long long>{}((client << 32) ^ server);
    }
};

} // namespace endweave
