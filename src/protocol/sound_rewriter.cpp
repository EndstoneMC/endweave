/// SoundRewriter implementation.

#include "endweave/protocol/sound_rewriter.h"

#include <algorithm>

#include "endweave/codec/types/actor_data.h"
#include "endweave/codec/types/item.h"
#include "endweave/codec/types/primitives.h"
#include "endweave/protocol/packet_ids.h"

namespace endweave {

void SoundRewriter::remap_actor_data(PacketWrapper& wrapper) {
    auto entries = wrapper.read<actor_data_list>();
    if (!entries) return;

    auto& items = *entries;

    // Filter dropped keys
    if (!dropped_keys_.empty()) {
        std::erase_if(items, [&](const ActorDataItem& e) {
            return dropped_keys_.contains(e.key);
        });
    }

    // Remap integer fields
    for (auto& entry : items) {
        auto it = int_remappers_.find(static_cast<int>(entry.key));
        if (it != int_remappers_.end() && (entry.type_id == 2 || entry.type_id == 7)) {
            if (entry.type_id == 2) {
                entry.value = static_cast<std::int32_t>(it->second(std::any_cast<std::int32_t>(entry.value)));
            } else {
                entry.value = static_cast<std::int64_t>(it->second(static_cast<int>(std::any_cast<std::int64_t>(entry.value))));
            }
        }
    }

    wrapper.write<actor_data_list>(items);
}

void SoundRewriter::rewrite_level_sound_event(PacketWrapper& wrapper) {
    auto event_id = wrapper.read<uvar_int>();
    if (!event_id) return;
    wrapper.write<uvar_int>(static_cast<std::uint32_t>(sound_remap_(static_cast<int>(*event_id))));
    wrapper.passthrough_all();
}

void SoundRewriter::rewrite_set_actor_data(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uvar_int64>(); // Target Runtime ID
    remap_actor_data(wrapper);
    wrapper.passthrough_all(); // Synched Properties, Tick
}

void SoundRewriter::rewrite_add_actor(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int64>(); // Target Actor ID
    (void)wrapper.passthrough<uvar_int64>(); // Target Runtime ID
    (void)wrapper.passthrough<string>();     // Actor Type
    (void)wrapper.passthrough<vec3>();       // Position
    (void)wrapper.passthrough<vec3>();       // Velocity
    (void)wrapper.passthrough<vec2>();       // Rotation
    (void)wrapper.passthrough<float_le>();   // Y Head Rotation
    (void)wrapper.passthrough<float_le>();   // Y Body Rotation
    auto attr_count = wrapper.passthrough<uvar_int>();
    if (attr_count) {
        for (std::uint32_t i = 0; i < *attr_count; ++i) {
            (void)wrapper.passthrough<string>();   // Attribute Name
            (void)wrapper.passthrough<float_le>(); // Min Value
            (void)wrapper.passthrough<float_le>(); // Current Value
            (void)wrapper.passthrough<float_le>(); // Max Value
        }
    }
    remap_actor_data(wrapper);
    wrapper.passthrough_all(); // Synched Properties, Actor Links
}

void SoundRewriter::rewrite_add_item_actor(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int64>();    // Target Actor ID
    (void)wrapper.passthrough<uvar_int64>();   // Target Runtime ID
    (void)wrapper.passthrough<item_instance>(); // Item
    (void)wrapper.passthrough<vec3>();          // Position
    (void)wrapper.passthrough<vec3>();          // Velocity
    remap_actor_data(wrapper);
    wrapper.passthrough_all(); // From Fishing?
}

void SoundRewriter::rewrite_add_player(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uuid>();           // UUID
    (void)wrapper.passthrough<string>();          // Player Name
    (void)wrapper.passthrough<uvar_int64>();      // Target Runtime ID
    (void)wrapper.passthrough<string>();          // Platform Chat Id
    (void)wrapper.passthrough<vec3>();            // Position
    (void)wrapper.passthrough<vec3>();            // Velocity
    (void)wrapper.passthrough<vec2>();            // Rotation
    (void)wrapper.passthrough<float_le>();        // Y-Head Rotation
    (void)wrapper.passthrough<item_instance>();   // Carried Item
    (void)wrapper.passthrough<var_int>();         // Player Game Type
    remap_actor_data(wrapper);
    wrapper.passthrough_all(); // Synched Properties, AbilitiesData, Actor Links
}

void SoundRewriter::register_on(Protocol& protocol) {
    auto id = [](PacketId p) { return static_cast<int>(p); };

    protocol.register_clientbound(id(PacketId::LEVEL_SOUND_EVENT),
        [this](PacketWrapper& w) { rewrite_level_sound_event(w); });
    protocol.register_clientbound(id(PacketId::SET_ACTOR_DATA),
        [this](PacketWrapper& w) { rewrite_set_actor_data(w); });
    protocol.register_clientbound(id(PacketId::ADD_ACTOR),
        [this](PacketWrapper& w) { rewrite_add_actor(w); });
    protocol.register_clientbound(id(PacketId::ADD_ITEM_ACTOR),
        [this](PacketWrapper& w) { rewrite_add_item_actor(w); });
    protocol.register_clientbound(id(PacketId::ADD_PLAYER),
        [this](PacketWrapper& w) { rewrite_add_player(w); });
}

} // namespace endweave
