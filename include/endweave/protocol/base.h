#pragma once

/// Base protocol - always-on handler for version detection and login rewriting.
///
/// See Also:
///     com.viaversion.viaversion.protocols.base.InitialBaseProtocol

#include <memory>

#include "endweave/protocol/protocol.h"

namespace endweave {

/// Create the base protocol that detects client version and rewrites login.
std::unique_ptr<Protocol> create_base_protocol(int server_protocol);

} // namespace endweave
