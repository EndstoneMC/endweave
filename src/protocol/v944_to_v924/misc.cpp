/// v944 -> v924 start_game, camera, voxel_shapes, data_driven_ui handlers.

#include "endweave/protocol/v944_to_v924/handlers.h"

#include "endweave/codec/types/camera.h"
#include "endweave/codec/types/level_settings.h"
#include "endweave/codec/types/nbt.h"
#include "endweave/codec/types/primitives.h"

namespace endweave::v944_to_v924 {

void rewrite_start_game(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<var_int64>();
    (void)wrapper.passthrough<uvar_int64>();
    (void)wrapper.passthrough<var_int>();
    (void)wrapper.passthrough<vec3>();
    (void)wrapper.passthrough<vec2>();

    (void)wrapper.map<level_settings_v944, level_settings_v924>();

    (void)wrapper.passthrough<string>();
    (void)wrapper.passthrough<string>();
    (void)wrapper.passthrough<string>();
    (void)wrapper.passthrough<bool_t>();
    (void)wrapper.passthrough<var_int>();
    (void)wrapper.passthrough<bool_t>();
    (void)wrapper.passthrough<int64_le>();
    (void)wrapper.passthrough<var_int>();

    auto block_prop_count = wrapper.passthrough<uvar_int>();
    if (block_prop_count) {
        for (std::uint32_t i = 0; i < *block_prop_count; ++i) {
            (void)wrapper.passthrough<string>();
            (void)wrapper.passthrough<named_compound_tag>();
        }
    }

    (void)wrapper.passthrough<string>();
    (void)wrapper.passthrough<bool_t>();
    (void)wrapper.passthrough<string>();
    (void)wrapper.passthrough<named_compound_tag>();
    (void)wrapper.passthrough<int64_le>();  // Checksum (no remap in reverse)
    (void)wrapper.passthrough<int64_le>();
    (void)wrapper.passthrough<int64_le>();
    (void)wrapper.passthrough<bool_t>();
    (void)wrapper.passthrough<bool_t>();
    (void)wrapper.passthrough<bool_t>();

    // Server join info divergence (v944 -> v924)
    auto has_sji = wrapper.passthrough<bool_t>();
    if (has_sji && *has_sji) {
        (void)wrapper.read<bool_t>(); // has gathering join information
        (void)wrapper.read<bool_t>(); // has client store entry point information
        (void)wrapper.read<bool_t>(); // has presence information
        wrapper.write<bool_t>(false); // has gathering (v924 form)
    }
}

void rewrite_camera_spline(PacketWrapper& wrapper) {
    auto spline_count = wrapper.passthrough<uvar_int>();
    if (spline_count) {
        for (std::uint32_t i = 0; i < *spline_count; ++i) {
            (void)wrapper.passthrough<string>();
            (void)wrapper.map<spline_instruction_v944, spline_instruction_v924>();
        }
    }
}

void rewrite_camera_instruction(PacketWrapper& wrapper) {
    auto has_set = wrapper.passthrough<bool_t>();
    if (has_set && *has_set) {
        (void)wrapper.passthrough<int_le>();
        auto has_ease = wrapper.passthrough<bool_t>();
        if (has_ease && *has_ease) {
            (void)wrapper.passthrough<byte_t>();
            (void)wrapper.passthrough<float_le>();
        }
        (void)wrapper.passthrough<optional_vec3>();
        (void)wrapper.passthrough<optional_vec2>();
        (void)wrapper.passthrough<optional_vec3>();
        (void)wrapper.passthrough<optional_vec2>();
        (void)wrapper.passthrough<optional_vec3>();
        (void)wrapper.passthrough<optional_bool>();
        (void)wrapper.passthrough<bool_t>();
    }

    (void)wrapper.passthrough<optional_bool>();

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

    auto has_target = wrapper.passthrough<bool_t>();
    if (has_target && *has_target) {
        (void)wrapper.passthrough<optional_vec3>();
        (void)wrapper.passthrough<int64_le>();
    }

    (void)wrapper.passthrough<optional_bool>();

    auto has_fov = wrapper.passthrough<bool_t>();
    if (has_fov && *has_fov) {
        (void)wrapper.passthrough<float_le>();
        (void)wrapper.passthrough<float_le>();
        (void)wrapper.passthrough<byte_t>();
        (void)wrapper.passthrough<bool_t>();
    }

    auto has_spline = wrapper.passthrough<bool_t>();
    if (has_spline && *has_spline) {
        (void)wrapper.map<spline_instruction_v944, spline_instruction_v924>();
    }
}

void rewrite_voxel_shapes(PacketWrapper& wrapper) {
    auto data = wrapper.read<remaining_bytes>();
    if (data && data->size() >= 2) {
        wrapper.writer().write_bytes(data->substr(0, data->size() - 2));
    }
}

void rewrite_show_screen(PacketWrapper& wrapper) {
    (void)wrapper.passthrough<string>();  // ScreenId
    (void)wrapper.read<int_le>();         // FormId (strip)
    auto has_data = wrapper.read<bool_t>();
    if (has_data && *has_data) {
        (void)wrapper.read<int_le>();     // DataInstanceId (strip)
    }
}

void rewrite_close_screen(PacketWrapper& wrapper) {
    auto has_form = wrapper.read<bool_t>();
    if (has_form && *has_form) {
        (void)wrapper.read<int_le>();     // FormId (strip)
    }
}

} // namespace endweave::v944_to_v924
