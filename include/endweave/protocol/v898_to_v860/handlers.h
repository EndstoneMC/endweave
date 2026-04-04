#pragma once

/// v898 -> v860 packet handlers.

#include "endweave/codec/wrapper.h"

namespace endweave::v898_to_v860 {

// actor_event
void rewrite_actor_event(PacketWrapper& wrapper);

// camera_aim_assist
void rewrite_camera_aim_assist_presets(PacketWrapper& wrapper);

// commands
void rewrite_available_commands(PacketWrapper& wrapper);
void rewrite_command_request(PacketWrapper& wrapper);
void rewrite_command_output(PacketWrapper& wrapper);

// event
void rewrite_event(PacketWrapper& wrapper);

// interact
void rewrite_interact(PacketWrapper& wrapper);

// mob_effect
void rewrite_mob_effect(PacketWrapper& wrapper);

// resource_pack_stack
void rewrite_resource_pack_stack(PacketWrapper& wrapper);

// start_game
void rewrite_start_game(PacketWrapper& wrapper);

// text (reuses v860_to_v898 text handlers)
void rewrite_text_clientbound(PacketWrapper& wrapper);
void rewrite_text_serverbound(PacketWrapper& wrapper);

} // namespace endweave::v898_to_v860
