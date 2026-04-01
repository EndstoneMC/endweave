#pragma once

#include <memory>

#include "endweave/protocol/protocol.h"

namespace endweave::v859_to_v860 {

inline constexpr int SERVER_PROTOCOL = 859;
inline constexpr int CLIENT_PROTOCOL = 860;

/// v859 and v860 share identical packet structures.
inline std::unique_ptr<Protocol> create_protocol() {
    return std::make_unique<Protocol>(SERVER_PROTOCOL, CLIENT_PROTOCOL);
}

} // namespace endweave::v859_to_v860
