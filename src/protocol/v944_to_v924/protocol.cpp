/// v944 -> v924 protocol factory.

#include "endweave/protocol/v944_to_v924/protocol.h"

#include "endweave/codec/types/enums.h"
#include "endweave/protocol/packet_ids.h"
#include "endweave/protocol/v944_to_v924/handlers.h"

namespace endweave::v944_to_v924 {

namespace {

int remap_sound(int v) {
    // Remap LevelSoundEvent from v944 -> v924 (collapse growth events)
    constexpr int UNDEFINED_V944 = 599;
    constexpr int UNDEFINED_V924 = 597;
    if (v >= UNDEFINED_V944)
        return v - (UNDEFINED_V944 - UNDEFINED_V924);
    if (v >= UNDEFINED_V924)
        return UNDEFINED_V924;
    return v;
}

} // namespace

ProtocolState create_protocol() {
    auto id = [](PacketId p) { return static_cast<int>(p); };

    auto p = std::make_unique<Protocol>(SERVER_PROTOCOL, CLIENT_PROTOCOL, "v944_to_v924");

    // Cancel packets unknown to v924
    p->cancel_clientbound({
        id(PacketId::LOCATOR_BAR),
        id(PacketId::SYNC_WORLD_CLOCKS),
        id(PacketId::CLIENTBOUND_ATTRIBUTE_LAYER_SYNC),
    });

    // Clientbound: BlockPos -> NetworkBlockPos
    p->register_clientbound(id(PacketId::START_GAME), rewrite_start_game);
    p->register_clientbound(id(PacketId::UPDATE_BLOCK), rewrite_first_block_to_net);
    p->register_clientbound(id(PacketId::TILE_EVENT), rewrite_tile_event);
    p->register_clientbound(id(PacketId::SET_SPAWN_POSITION), rewrite_set_spawn_position);
    p->register_clientbound(id(PacketId::BLOCK_ACTOR_DATA), rewrite_first_block_to_net);
    p->register_clientbound(id(PacketId::UPDATE_BLOCK_SYNCED), rewrite_first_block_to_net);
    p->register_clientbound(id(PacketId::LECTERN_UPDATE), rewrite_lectern_update_clientbound);
    p->register_serverbound(id(PacketId::LECTERN_UPDATE), rewrite_lectern_update_serverbound);
    p->register_clientbound(id(PacketId::ADD_VOLUME_ENTITY), rewrite_add_volume_entity);
    p->register_clientbound(id(PacketId::UPDATE_SUB_CHUNK_BLOCKS), rewrite_update_sub_chunk_blocks);
    p->register_clientbound(id(PacketId::OPEN_SIGN), rewrite_first_block_to_net);
    p->register_clientbound(id(PacketId::PLAY_SOUND), rewrite_play_sound);
    p->register_clientbound(id(PacketId::MAP_DATA), rewrite_map_data);

    // Clientbound: other
    p->register_clientbound(id(PacketId::PLAYER_CLIENT_INPUT_PERMISSIONS), rewrite_update_client_input_locks);
    p->register_clientbound(id(PacketId::VOXEL_SHAPES), rewrite_voxel_shapes);
    p->register_clientbound(id(PacketId::CLIENTBOUND_DATA_DRIVEN_UI_SHOW_SCREEN), rewrite_show_screen);
    p->register_clientbound(id(PacketId::CLIENTBOUND_DATA_DRIVEN_UI_CLOSE_ALL_SCREENS), rewrite_close_screen);
    p->register_clientbound(id(PacketId::CAMERA_INSTRUCTION), rewrite_camera_instruction);
    p->register_clientbound(id(PacketId::CAMERA_SPLINE), rewrite_camera_spline);
    p->register_clientbound(id(PacketId::CONTAINER_OPEN), rewrite_container_open);

    // Serverbound: NetworkBlockPos -> BlockPos
    p->register_serverbound(id(PacketId::INVENTORY_TRANSACTION), rewrite_inventory_transaction);
    p->register_serverbound(id(PacketId::PLAYER_ACTION), rewrite_player_action);
    p->register_serverbound(id(PacketId::COMMAND_BLOCK_UPDATE), rewrite_command_block_update);
    p->register_serverbound(id(PacketId::STRUCTURE_BLOCK_UPDATE), rewrite_structure_block_update);
    p->register_serverbound(id(PacketId::STRUCTURE_TEMPLATE_DATA_EXPORT_REQUEST), rewrite_structure_template_data_request);
    p->register_serverbound(id(PacketId::BLOCK_ACTOR_DATA), rewrite_first_net_to_block);
    p->register_serverbound(id(PacketId::ANVIL_DAMAGE), rewrite_anvil_damage);

    // SoundRewriter
    SoundRewriter sound(remap_sound,
        {{static_cast<int>(ActorDataIDs::HEARTBEAT_SOUND_EVENT), remap_sound}});
    sound.register_on(*p);

    return {std::move(p), std::move(sound)};
}

} // namespace endweave::v944_to_v924
