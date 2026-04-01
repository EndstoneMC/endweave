/// v924 -> v944 block position handlers.
///
/// Clientbound: NetworkBlockPos -> BlockPos
/// Serverbound: BlockPos -> NetworkBlockPos

#include "endweave/protocol/v924_to_v944/handlers.h"

#include "endweave/codec/types/enums.h"
#include "endweave/codec/types/inventory.h"
#include "endweave/codec/types/item.h"
#include "endweave/codec/types/primitives.h"
#include "endweave/codec/types/structure.h"

namespace endweave::v924_to_v944 {

namespace {
constexpr int NOTE_BLOCK_EVENT = 0;
constexpr int TRUMPET_INSERTION_POINT = 16;
constexpr int TRUMPET_ID_SHIFT = 4;
} // namespace

// -- Clientbound: NetworkBlockPos -> BlockPos --

void rewrite_first_net_block_to_block(PacketWrapper& wrapper) {
    (void)wrapper.map<network_block_pos, block_pos>();
}

void rewrite_lectern_update_clientbound(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<byte_t>();  // New page to show
    (void)wrapper.passthrough<byte_t>();  // Total Pages
    (void)wrapper.map<network_block_pos, block_pos>();
}

void rewrite_lectern_update_serverbound(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.passthrough<byte_t>();
    (void)wrapper.map<block_pos, network_block_pos>();
}

void rewrite_tile_event(PacketWrapper& wrapper) {
    (void)wrapper.map<network_block_pos, block_pos>();  // Block Position
    auto event_type = wrapper.passthrough<var_int>();    // Event Type
    auto event_data = wrapper.read<var_int>();           // Event Value
    if (event_type && event_data) {
        auto val = *event_data;
        if (*event_type == NOTE_BLOCK_EVENT && val >= TRUMPET_INSERTION_POINT)
            val += TRUMPET_ID_SHIFT;
        wrapper.write<var_int>(val);
    }
}

void rewrite_set_spawn_position(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int>();                // Spawn Position Type
    (void)wrapper.map<network_block_pos, block_pos>();   // Block Position
    (void)wrapper.passthrough<var_int>();                // Dimension type
    (void)wrapper.map<network_block_pos, block_pos>();   // Spawn Block Pos
}

void rewrite_add_volume_entity(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uvar_int>();               // Entity Network Id
    (void)wrapper.map<network_block_pos, block_pos>();   // Min Bounds
    (void)wrapper.map<network_block_pos, block_pos>();   // Max Bounds
}

void rewrite_update_sub_chunk_blocks(PacketWrapper& wrapper) {
    (void)wrapper.map<network_block_pos, block_pos>();   // Sub Chunk Block Position

    // Blocks Changed - Standards
    auto blocks_count = wrapper.passthrough<uvar_int>();
    if (blocks_count) {
        for (std::uint32_t i = 0; i < *blocks_count; ++i) {
            (void)wrapper.map<network_block_pos, block_pos>();
            (void)wrapper.passthrough<uvar_int>();   // Runtime Id
            (void)wrapper.passthrough<uvar_int>();   // Update Flags
            (void)wrapper.passthrough<uvar_int64>(); // Sync Message - Entity Unique ID
            (void)wrapper.passthrough<uvar_int>();   // Sync Message - Message
        }
    }

    // Blocks Changed - Extras
    auto extra_count = wrapper.passthrough<uvar_int>();
    if (extra_count) {
        for (std::uint32_t i = 0; i < *extra_count; ++i) {
            (void)wrapper.map<network_block_pos, block_pos>();
            (void)wrapper.passthrough<uvar_int>();
            (void)wrapper.passthrough<uvar_int>();
            (void)wrapper.passthrough<uvar_int64>();
            (void)wrapper.passthrough<uvar_int>();
        }
    }
}

void rewrite_play_sound(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<string>();                 // Name
    (void)wrapper.map<network_block_pos, block_pos>();   // Position
}

void rewrite_map_data(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int64>();   // Map ID
    auto types = wrapper.passthrough<uvar_int>();  // Type Flags
    (void)wrapper.passthrough<byte_t>();      // Dimension
    (void)wrapper.passthrough<bool_t>();      // Is Locked Map?
    (void)wrapper.passthrough<block_pos>();   // Map Origin

    if (!types) return;

    if (*types & static_cast<std::uint32_t>(ClientboundMapItemDataType::Creation)) {
        // Map ID List: uvarint count + varint64 entries
        auto count = wrapper.passthrough<uvar_int>();
        if (count) {
            for (std::uint32_t i = 0; i < *count; ++i)
                (void)wrapper.passthrough<var_int64>();
        }
    }

    if (*types & (static_cast<std::uint32_t>(ClientboundMapItemDataType::Creation) |
                  static_cast<std::uint32_t>(ClientboundMapItemDataType::DecorationUpdate) |
                  static_cast<std::uint32_t>(ClientboundMapItemDataType::TextureUpdate))) {
        (void)wrapper.passthrough<byte_t>(); // Scale
    }

    if (*types & static_cast<std::uint32_t>(ClientboundMapItemDataType::DecorationUpdate)) {
        auto obj_count = wrapper.passthrough<uvar_int>();
        if (obj_count) {
            for (std::uint32_t i = 0; i < *obj_count; ++i) {
                auto obj_type = wrapper.passthrough<int_le>();
                if (obj_type && *obj_type == static_cast<std::int32_t>(MapItemTrackedActorType::Entity)) {
                    (void)wrapper.passthrough<var_int64>();
                } else if (obj_type && *obj_type == static_cast<std::int32_t>(MapItemTrackedActorType::BlockEntity)) {
                    (void)wrapper.map<network_block_pos, block_pos>();
                }
            }
        }
    }
}

void rewrite_update_client_input_locks(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uvar_int>(); // Input Lock ComponentData
    (void)wrapper.read<float_le>();        // discard Server Pos.X
    (void)wrapper.read<float_le>();        // discard Server Pos.Y
    (void)wrapper.read<float_le>();        // discard Server Pos.Z
}

// -- Serverbound: BlockPos -> NetworkBlockPos --

void rewrite_first_block_to_net_block(PacketWrapper& wrapper) {
    (void)wrapper.map<block_pos, network_block_pos>();
}

void rewrite_inventory_transaction(PacketWrapper& wrapper) {
    auto legacy_request_id = wrapper.passthrough<var_int>();
    if (legacy_request_id && *legacy_request_id != 0) {
        auto slot_count = wrapper.passthrough<uvar_int>();
        if (slot_count) {
            for (std::uint32_t i = 0; i < *slot_count; ++i) {
                (void)wrapper.passthrough<byte_t>();   // Container Enum
                auto slots_len = wrapper.passthrough<uvar_int>();
                if (slots_len) {
                    for (std::uint32_t j = 0; j < *slots_len; ++j)
                        (void)wrapper.passthrough<byte_t>(); // Slot
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

    // UseItemTransactionData
    (void)wrapper.passthrough<uvar_int>();                // ActionType
    (void)wrapper.passthrough<uvar_int>();                // TriggerType
    (void)wrapper.map<block_pos, network_block_pos>();    // BlockPosition
    (void)wrapper.passthrough<var_int>();                 // BlockFace
    (void)wrapper.passthrough<var_int>();                 // HotBarSlot
    (void)wrapper.passthrough<item_instance>();           // HeldItem
    (void)wrapper.passthrough<vec3>();                    // Position
    (void)wrapper.passthrough<vec3>();                    // ClickedPosition
    (void)wrapper.passthrough<uvar_int>();                // BlockRuntimeID
    (void)wrapper.passthrough<uvar_int>();                // ClientPrediction
    (void)wrapper.read<byte_t>();                         // ClientCooldownState (strip)
}

void rewrite_player_action(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<uvar_int64>(); // Player Runtime ID
    (void)wrapper.passthrough<var_int>();     // Action
    (void)wrapper.map<block_pos, network_block_pos>(); // Block Position
    (void)wrapper.map<block_pos, network_block_pos>(); // Result Pos
}

void rewrite_container_open(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<byte_t>(); // Container Id
    (void)wrapper.passthrough<byte_t>(); // Container Type
    (void)wrapper.map<network_block_pos, block_pos>(); // Position
}

void rewrite_structure_block_update(PacketWrapper& wrapper) {
    (void)wrapper.map<block_pos, network_block_pos>(); // Block Position
    // StructureEditorData
    (void)wrapper.passthrough<string>();  // Name
    (void)wrapper.passthrough<string>();  // DataField
    (void)wrapper.passthrough<bool_t>();  // IncludePlayers
    (void)wrapper.passthrough<bool_t>();  // ShowBoundingBox
    (void)wrapper.passthrough<var_int>(); // StructureBlockType
    (void)wrapper.map<structure_settings_v944, structure_settings_v924>();
}

void rewrite_command_block_update(PacketWrapper& wrapper) {
    auto is_block = wrapper.passthrough<bool_t>();
    if (is_block && *is_block) {
        (void)wrapper.map<block_pos, network_block_pos>();
    }
}

void rewrite_structure_template_data_request(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<string>();  // Structure Name
    (void)wrapper.map<block_pos, network_block_pos>(); // Structure Position
    (void)wrapper.map<structure_settings_v944, structure_settings_v924>();
}

void rewrite_anvil_damage(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<byte_t>(); // Damage Amount
    (void)wrapper.map<block_pos, network_block_pos>(); // Block Position
}

} // namespace endweave::v924_to_v944
