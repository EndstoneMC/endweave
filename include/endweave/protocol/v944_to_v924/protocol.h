#pragma once

#include <memory>

#include "endweave/protocol/protocol.h"
#include "endweave/protocol/sound_rewriter.h"

namespace endweave::v944_to_v924 {

inline constexpr int SERVER_PROTOCOL = 944;
inline constexpr int CLIENT_PROTOCOL = 924;

struct ProtocolState {
    std::unique_ptr<Protocol> protocol;
    SoundRewriter sound_rewriter;
};

/// Create the v944 -> v924 protocol with all handlers registered.
ProtocolState create_protocol();

} // namespace endweave::v944_to_v924
