#pragma once

#include <memory>

#include "endweave/protocol/protocol.h"
#include "endweave/protocol/sound_rewriter.h"

namespace endweave::v898_to_v860 {

inline constexpr int SERVER_PROTOCOL = 898;
inline constexpr int CLIENT_PROTOCOL = 860;

struct ProtocolState {
    std::unique_ptr<Protocol> protocol;
    SoundRewriter sound_rewriter;
};

/// Create the v898 -> v860 protocol with all handlers registered.
ProtocolState create_protocol();

} // namespace endweave::v898_to_v860
