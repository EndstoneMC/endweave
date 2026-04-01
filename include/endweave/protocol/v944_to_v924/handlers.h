#pragma once

/// v944 -> v924 packet handlers.

#include "endweave/codec/wrapper.h"

namespace endweave::v944_to_v924 {

// block_pos handlers (reverse of v924_to_v944)
void rewrite_first_block_to_net(PacketWrapper& wrapper);
void rewrite_first_net_to_block(PacketWrapper& wrapper);
void rewrite_lectern_update_clientbound(PacketWrapper& wrapper);
void rewrite_lectern_update_serverbound(PacketWrapper& wrapper);
void rewrite_tile_event(PacketWrapper& wrapper);
void rewrite_set_spawn_position(PacketWrapper& wrapper);
void rewrite_add_volume_entity(PacketWrapper& wrapper);
void rewrite_update_sub_chunk_blocks(PacketWrapper& wrapper);
void rewrite_play_sound(PacketWrapper& wrapper);
void rewrite_map_data(PacketWrapper& wrapper);
void rewrite_update_client_input_locks(PacketWrapper& wrapper);
void rewrite_inventory_transaction(PacketWrapper& wrapper);
void rewrite_player_action(PacketWrapper& wrapper);
void rewrite_container_open(PacketWrapper& wrapper);
void rewrite_structure_block_update(PacketWrapper& wrapper);
void rewrite_command_block_update(PacketWrapper& wrapper);
void rewrite_structure_template_data_request(PacketWrapper& wrapper);
void rewrite_anvil_damage(PacketWrapper& wrapper);

// other handlers
void rewrite_start_game(PacketWrapper& wrapper);
void rewrite_voxel_shapes(PacketWrapper& wrapper);
void rewrite_show_screen(PacketWrapper& wrapper);
void rewrite_close_screen(PacketWrapper& wrapper);
void rewrite_camera_instruction(PacketWrapper& wrapper);
void rewrite_camera_spline(PacketWrapper& wrapper);

} // namespace endweave::v944_to_v924
