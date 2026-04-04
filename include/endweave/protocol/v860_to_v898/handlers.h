#pragma once

/// v860 -> v898 packet handlers.

#include "endweave/codec/wrapper.h"

namespace endweave::v860_to_v898 {

// actor_event
void rewrite_actor_event(PacketWrapper& wrapper);

// animate
void rewrite_animate_clientbound(PacketWrapper& wrapper);
void rewrite_animate_serverbound(PacketWrapper& wrapper);

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

// text
void rewrite_text_clientbound(PacketWrapper& wrapper);
void rewrite_text_serverbound(PacketWrapper& wrapper);

} // namespace endweave::v860_to_v898
