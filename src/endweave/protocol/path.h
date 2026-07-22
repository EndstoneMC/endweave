#pragma once

#include "endweave/protocol/direction.h"

#include <cstddef>
#include <functional>
#include <vector>

namespace endweave {

class AbstractProtocol;

/**
 * One hop of a resolved path, mirroring ViaVersion's ProtocolPathEntry.
 *
 * The protocol is the newer of the hop's two nodes -- the one owning the wire diff -- and the
 * step picks which of its tables this traversal uses. As in ViaVersion the entry carries the
 * version the stream is in *after* it: the input is the previous entry's output.
 */
struct ProtocolPathEntry {
    const AbstractProtocol *protocol = nullptr;
    Step step = Step::Upgrade;
    int output_protocol_version = 0;

    friend bool operator==(const ProtocolPathEntry &, const ProtocolPathEntry &) = default;
};

/**
 * A resolved path, in serverbound order: client version first, server version last.
 */
using ProtocolPath = std::vector<ProtocolPathEntry>;

/**
 * The (client version, server version) pair a cached path is keyed by.
 */
struct ProtocolPathKey {
    int client_protocol_version = 0;
    int server_protocol_version = 0;

    friend bool operator==(const ProtocolPathKey &, const ProtocolPathKey &) = default;
};

/**
 * Hashes a ProtocolPathKey, for the path cache.
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
