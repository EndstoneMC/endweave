/// v944 -> v924 block position handlers.
///
/// Clientbound: BlockPos -> NetworkBlockPos
/// Serverbound: NetworkBlockPos -> BlockPos

#include "endweave/protocol/v944_to_v924/handlers.h"

#include "endweave/codec/types/enums.h"
#include "endweave/codec/types/inventory.h"
#include "endweave/codec/types/item.h"
#include "endweave/codec/types/primitives.h"
#include "endweave/codec/types/structure.h"

namespace endweave::v944_to_v924 {

namespace {
constexpr int NOTE_BLOCK_EVENT = 0;
constexpr int TRUMPET_INSERTION_POINT = 16;
constexpr int TRUMPET_RANGE_END = 20;  // 16..19 are the inserted Trumpet IDs
constexpr int TRUMPET_ID_SHIFT = 4;
} // namespace

// -- Clientbound: BlockPos -> NetworkBlockPos --

void rewrite_first_block_to_net(PacketWrapper& wrapper) {
    (void)wrapper.map<block_pos, network_block_pos>();
}

void rewrite_lectern_update_clientbound(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.map<block_pos, network_block_pos>();
}

void rewrite_lectern_update_serverbound(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.map<network_block_pos, block_pos>();
}

void rewrite_tile_event(PacketWrapper& wrapper) {
    (void)wrapper.map<block_pos, network_block_pos>();
    auto event_type = wrapper.passthrough<var_int>();
    auto event_data = wrapper.read<var_int>();
    if (event_type && event_data) {
        auto val = *event_data;
        if (*event_type == NOTE_BLOCK_EVENT) {
            if (val >= TRUMPET_INSERTION_POINT && val < TRUMPET_RANGE_END)
                val = TRUMPET_INSERTION_POINT; // collapse Trumpet variants
            else if (val >= TRUMPET_RANGE_END)
                val -= TRUMPET_ID_SHIFT;
        }
        wrapper.write<var_int>(val);
    }
}

void rewrite_set_spawn_position(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int>();
    (void)wrapper.map<block_pos, network_block_pos>();
    (void)wrapper.passthrough<var_int>();
    (void)wrapper.map<block_pos, network_block_pos>();
}

void rewrite_add_volume_entity(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uvar_int>();
    (void)wrapper.map<block_pos, network_block_pos>();
    (void)wrapper.map<block_pos, network_block_pos>();
}

void rewrite_update_sub_chunk_blocks(PacketWrapper& wrapper) {
    (void)wrapper.map<block_pos, network_block_pos>();

    auto blocks_count = wrapper.passthrough<uvar_int>();
    if (blocks_count) {
        for (std::uint32_t i = 0; i < *blocks_count; ++i) {
            (void)wrapper.map<block_pos, network_block_pos>();
            (void)wrapper.passthrough<uvar_int>();
            (void)wrapper.passthrough<uvar_int>();
            (void)wrapper.passthrough<uvar_int64>();
            (void)wrapper.passthrough<uvar_int>();
        }
    }

    auto extra_count = wrapper.passthrough<uvar_int>();
    if (extra_count) {
        for (std::uint32_t i = 0; i < *extra_count; ++i) {
            (void)wrapper.map<block_pos, network_block_pos>();
            (void)wrapper.passthrough<uvar_int>();
            (void)wrapper.passthrough<uvar_int>();
            (void)wrapper.passthrough<uvar_int64>();
            (void)wrapper.passthrough<uvar_int>();
        }
    }
}

void rewrite_play_sound(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<string>();
    (void)wrapper.map<block_pos, network_block_pos>();
}

void rewrite_map_data(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int64>();
    auto types = wrapper.passthrough<uvar_int>();
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.passthrough<bool_t>();
    (void)wrapper.passthrough<block_pos>();  // Map Origin (already BlockPos in both)

    if (!types) return;

    if (*types & static_cast<std::uint32_t>(ClientboundMapItemDataType::Creation)) {
        auto count = wrapper.passthrough<uvar_int>();
        if (count) {
            for (std::uint32_t i = 0; i < *count; ++i)
                (void)wrapper.passthrough<var_int64>();
        }
    }

    if (*types & (static_cast<std::uint32_t>(ClientboundMapItemDataType::Creation) |
                  static_cast<std::uint32_t>(ClientboundMapItemDataType::DecorationUpdate) |
                  static_cast<std::uint32_t>(ClientboundMapItemDataType::TextureUpdate))) {
        (void)wrapper.passthrough<byte_t>();
    }

    if (*types & static_cast<std::uint32_t>(ClientboundMapItemDataType::DecorationUpdate)) {
        auto obj_count = wrapper.passthrough<uvar_int>();
        if (obj_count) {
            for (std::uint32_t i = 0; i < *obj_count; ++i) {
                auto obj_type = wrapper.passthrough<int_le>();
                if (obj_type && *obj_type == static_cast<std::int32_t>(MapItemTrackedActorType::Entity)) {
                    (void)wrapper.passthrough<var_int64>();
                } else if (obj_type && *obj_type == static_cast<std::int32_t>(MapItemTrackedActorType::BlockEntity)) {
                    (void)wrapper.map<block_pos, network_block_pos>();
                }
            }
        }
    }
}

void rewrite_update_client_input_locks(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uvar_int>();
    // Append Server Pos (3 floats, set to 0.0) not present in v944
    wrapper.write<float_le>(0.0f);
    wrapper.write<float_le>(0.0f);
    wrapper.write<float_le>(0.0f);
}

// -- Serverbound: NetworkBlockPos -> BlockPos --

void rewrite_first_net_to_block(PacketWrapper& wrapper) {
    (void)wrapper.map<network_block_pos, block_pos>();
}

void rewrite_inventory_transaction(PacketWrapper& wrapper) {
    auto legacy_request_id = wrapper.passthrough<var_int>();
    if (legacy_request_id && *legacy_request_id != 0) {
        auto slot_count = wrapper.passthrough<uvar_int>();
        if (slot_count) {
            for (std::uint32_t i = 0; i < *slot_count; ++i) {
                (void)wrapper.passthrough<byte_t>();
                auto slots_len = wrapper.passthrough<uvar_int>();
                if (slots_len) {
                    for (std::uint32_t j = 0; j < *slots_len; ++j)
                        (void)wrapper.passthrough<byte_t>();
                }
            }
        }
    }

    auto transaction_type = wrapper.passthrough<uvar_int>();
    auto action_count = wrapper.passthrough<uvar_int>();
    if (action_count) {
        for (std::uint32_t i = 0; i < *action_count; ++i)
            (void)wrapper.passthrough<inventory_action>();
    }

    if (!transaction_type ||
        *transaction_type != static_cast<std::uint32_t>(ComplexInventoryTransactionType::ItemUseTransaction))
        return;

    (void)wrapper.passthrough<uvar_int>();
    (void)wrapper.passthrough<uvar_int>();
    (void)wrapper.map<network_block_pos, block_pos>();
    (void)wrapper.passthrough<var_int>();
    (void)wrapper.passthrough<var_int>();
    (void)wrapper.passthrough<item_instance>();
    (void)wrapper.passthrough<vec3>();
    (void)wrapper.passthrough<vec3>();
    (void)wrapper.passthrough<uvar_int>();
    (void)wrapper.passthrough<uvar_int>();
    wrapper.write<byte_t>(0); // Add ClientCooldownState (v944 expects it)
}

void rewrite_player_action(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uvar_int64>();
    (void)wrapper.passthrough<var_int>();
    (void)wrapper.map<network_block_pos, block_pos>();
    (void)wrapper.map<network_block_pos, block_pos>();
}

void rewrite_container_open(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.map<block_pos, network_block_pos>();
}

void rewrite_structure_block_update(PacketWrapper& wrapper) {
    (void)wrapper.map<network_block_pos, block_pos>();
    (void)wrapper.passthrough<string>();
    (void)wrapper.passthrough<string>();
    (void)wrapper.passthrough<bool_t>();
    (void)wrapper.passthrough<bool_t>();
    (void)wrapper.passthrough<var_int>();
    (void)wrapper.map<structure_settings_v924, structure_settings_v944>();
}

void rewrite_command_block_update(PacketWrapper& wrapper) {
    auto is_block = wrapper.passthrough<bool_t>();
    if (is_block && *is_block) {
        (void)wrapper.map<network_block_pos, block_pos>();
    }
}

void rewrite_structure_template_data_request(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<string>();
    (void)wrapper.map<network_block_pos, block_pos>();
    (void)wrapper.map<structure_settings_v924, structure_settings_v944>();
}

void rewrite_anvil_damage(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.map<network_block_pos, block_pos>();
}

} // namespace endweave::v944_to_v924
