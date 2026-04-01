#pragma once

#include <memory>

#include "endweave/protocol/protocol.h"

namespace endweave::v860_to_v859 {

inline constexpr int SERVER_PROTOCOL = 860;
inline constexpr int CLIENT_PROTOCOL = 859;

/// v859 and v860 share identical packet structures.
inline std::unique_ptr<Protocol> create_protocol() {
    return std::make_unique<Protocol>(SERVER_PROTOCOL, CLIENT_PROTOCOL);
}

} // namespace endweave::v860_to_v859
