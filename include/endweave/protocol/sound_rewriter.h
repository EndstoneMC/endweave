#pragma once

/// Reusable sound and ActorData remapping for protocol translation.
///
/// See Also:
///     com.viaversion.viaversion.rewriter.SoundRewriter

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "endweave/codec/wrapper.h"
#include "endweave/protocol/protocol.h"

namespace endweave {

using SoundRemap = std::function<int(int)>;

class SoundRewriter {
public:
    SoundRewriter(SoundRemap sound_remap,
                  std::unordered_map<int, SoundRemap> actor_data_int_remappers = {},
                  std::unordered_set<int> dropped_actor_data_keys = {})
        : sound_remap_(std::move(sound_remap))
        , int_remappers_(std::move(actor_data_int_remappers))
        , dropped_keys_(std::move(dropped_actor_data_keys)) {}

    /// Register all clientbound sound/ActorData handlers on the protocol.
    void register_on(Protocol& protocol);

private:
    void remap_actor_data(PacketWrapper& wrapper);
    void rewrite_level_sound_event(PacketWrapper& wrapper);
    void rewrite_set_actor_data(PacketWrapper& wrapper);
    void rewrite_add_actor(PacketWrapper& wrapper);
    void rewrite_add_item_actor(PacketWrapper& wrapper);
    void rewrite_add_player(PacketWrapper& wrapper);

    SoundRemap sound_remap_;
    std::unordered_map<int, SoundRemap> int_remappers_;
    std::unordered_set<int> dropped_keys_;
};

} // namespace endweave
