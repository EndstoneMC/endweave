#pragma once

#include <memory>

#include "endweave/protocol/protocol.h"
#include "endweave/protocol/sound_rewriter.h"

namespace endweave::v924_to_v898 {

inline constexpr int SERVER_PROTOCOL = 924;
inline constexpr int CLIENT_PROTOCOL = 898;

struct ProtocolState {
    std::unique_ptr<Protocol> protocol;
    SoundRewriter sound_rewriter;
};

/// Create the v924 -> v898 protocol with all handlers registered.
ProtocolState create_protocol();

} // namespace endweave::v924_to_v898
