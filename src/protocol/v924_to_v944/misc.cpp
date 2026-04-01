/// v924 -> v944 start_game, camera, voxel_shapes, data_driven_ui handlers.

#include "endweave/protocol/v924_to_v944/handlers.h"

#include "endweave/codec/types/camera.h"
#include "endweave/codec/types/level_settings.h"
#include "endweave/codec/types/nbt.h"
#include "endweave/codec/types/primitives.h"

namespace endweave::v924_to_v944 {

void rewrite_start_game(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int64>();   // Entity ID
    (void)wrapper.passthrough<uvar_int64>(); // Runtime ID
    (void)wrapper.passthrough<var_int>();     // Game Type
    (void)wrapper.passthrough<vec3>();        // Position
    (void)wrapper.passthrough<vec2>();        // Rotation

    (void)wrapper.map<level_settings_v924, level_settings_v944>();

    (void)wrapper.passthrough<string>();     // Level ID
    (void)wrapper.passthrough<string>();     // Level Name
    (void)wrapper.passthrough<string>();     // Template Content Identity
    (void)wrapper.passthrough<bool_t>();     // Is Trial?
    (void)wrapper.passthrough<var_int>();    // Movement Settings.RewindHistorySize
    (void)wrapper.passthrough<bool_t>();     // Movement Settings.ServerAuthBlockBreaking
    (void)wrapper.passthrough<int64_le>();   // Level Current Time
    (void)wrapper.passthrough<var_int>();    // Enchantment Seed

    auto block_prop_count = wrapper.passthrough<uvar_int>();
    if (block_prop_count) {
        for (std::uint32_t i = 0; i < *block_prop_count; ++i) {
            (void)wrapper.passthrough<string>();
            (void)wrapper.passthrough<named_compound_tag>();
        }
    }

    (void)wrapper.passthrough<string>();             // Multiplayer Correlation Id
    (void)wrapper.passthrough<bool_t>();             // Enable Item Stack Net Manager
    (void)wrapper.passthrough<string>();             // Server version
    (void)wrapper.passthrough<named_compound_tag>(); // Player Property Data
    (void)wrapper.read<int64_le>();                  // Server Block Type Registry Checksum
    wrapper.write<int64_le>(0);                      // zero checksum to skip validation
    (void)wrapper.passthrough<int64_le>();            // World Template ID (MSB)
    (void)wrapper.passthrough<int64_le>();            // World Template ID (LSB)
    (void)wrapper.passthrough<bool_t>();             // Server Enabled ClientSide Generation
    (void)wrapper.passthrough<bool_t>();             // BlockNetworkIds Are Hashes
    (void)wrapper.passthrough<bool_t>();             // NetworkPermissions

    // Server join info divergence
    auto has_sji = wrapper.passthrough<bool_t>();
    if (has_sji && *has_sji) {
        auto has_gathering = wrapper.read<bool_t>();
        if (has_gathering && *has_gathering) {
            for (int i = 0; i < 7; ++i) (void)wrapper.read<string>();
        }
        wrapper.write<bool_t>(false); // has gathering join information
        wrapper.write<bool_t>(false); // has client store entry point information
        wrapper.write<bool_t>(false); // has presence information
    }
}

void rewrite_camera_spline(PacketWrapper& wrapper) {
    auto spline_count = wrapper.passthrough<uvar_int>();
    if (spline_count) {
        for (std::uint32_t i = 0; i < *spline_count; ++i) {
            (void)wrapper.passthrough<string>(); // name
            (void)wrapper.map<spline_instruction_v924, spline_instruction_v944>();
        }
    }
}

void rewrite_camera_instruction(PacketWrapper& wrapper) {
    // optional Set
    auto has_set = wrapper.passthrough<bool_t>();
    if (has_set && *has_set) {
        (void)wrapper.passthrough<int_le>();    // preset
        auto has_ease = wrapper.passthrough<bool_t>();
        if (has_ease && *has_ease) {
            (void)wrapper.passthrough<byte_t>();    // type
            (void)wrapper.passthrough<float_le>();  // time
        }
        (void)wrapper.passthrough<optional_vec3>(); // pos
        (void)wrapper.passthrough<optional_vec2>(); // rot
        (void)wrapper.passthrough<optional_vec3>(); // facing
        (void)wrapper.passthrough<optional_vec2>(); // view_offset
        (void)wrapper.passthrough<optional_vec3>(); // entity_offset
        (void)wrapper.passthrough<optional_bool>(); // default
        (void)wrapper.passthrough<bool_t>();         // removeIgnoreStartingValuesComponent
    }

    (void)wrapper.passthrough<optional_bool>(); // Clear

    // optional Fade
    auto has_fade = wrapper.passthrough<bool_t>();
    if (has_fade && *has_fade) {
        auto has_time = wrapper.passthrough<bool_t>();
        if (has_time && *has_time) {
            (void)wrapper.passthrough<float_le>();
            (void)wrapper.passthrough<float_le>();
            (void)wrapper.passthrough<float_le>();
        }
        auto has_color = wrapper.passthrough<bool_t>();
        if (has_color && *has_color) {
            (void)wrapper.passthrough<float_le>();
            (void)wrapper.passthrough<float_le>();
            (void)wrapper.passthrough<float_le>();
        }
    }

    // optional Target
    auto has_target = wrapper.passthrough<bool_t>();
    if (has_target && *has_target) {
        (void)wrapper.passthrough<optional_vec3>();
        (void)wrapper.passthrough<int64_le>();
    }

    (void)wrapper.passthrough<optional_bool>(); // RemoveTarget

    // optional FieldOfView
    auto has_fov = wrapper.passthrough<bool_t>();
    if (has_fov && *has_fov) {
        (void)wrapper.passthrough<float_le>();
        (void)wrapper.passthrough<float_le>();
        (void)wrapper.passthrough<byte_t>();
        (void)wrapper.passthrough<bool_t>();
    }

    // optional Spline
    auto has_spline = wrapper.passthrough<bool_t>();
    if (has_spline && *has_spline) {
        (void)wrapper.map<spline_instruction_v924, spline_instruction_v944>();
    }
}

void rewrite_voxel_shapes(PacketWrapper& wrapper) {
    wrapper.passthrough_all();
    wrapper.write<uvar_int>(0); // Custom Shape Count
}

void rewrite_show_screen(PacketWrapper& wrapper) {
    wrapper.passthrough_all();
    wrapper.write<var_int>(0);                     // FormId
    wrapper.write<string>(std::string(""));        // DataInstanceId
}

void rewrite_close_all_screens(PacketWrapper& wrapper) {
    wrapper.write<bool_t>(false); // has FormId
}

} // namespace endweave::v924_to_v944
