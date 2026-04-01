/// v944 -> v924 miscellaneous handlers.

#include "endweave/protocol/v944_to_v924/handlers.h"

#include "endweave/codec/types/primitives.h"

namespace endweave::v944_to_v924 {

void rewrite_voxel_shapes(PacketWrapper& wrapper) {
    // Strip trailing Custom Shape Count field added in v944
    auto data = wrapper.read<remaining_bytes>();
    if (data && data->size() >= 2) {
        wrapper.writer().write_bytes(data->substr(0, data->size() - 2));
    }
}

void rewrite_show_screen(PacketWrapper& wrapper) {
    // Strip v944-only fields
    (void)wrapper.passthrough<string>();  // ScreenId
    (void)wrapper.read<int_le>();         // FormId (strip)
    // DataInstanceId: optional int (bool + int)
    auto has_data = wrapper.read<bool_t>();
    if (has_data && *has_data) {
        (void)wrapper.read<int_le>(); // strip DataInstanceId
    }
}

void rewrite_close_screen(PacketWrapper& wrapper) {
    // Strip v944 optional FormId
    auto has_form = wrapper.read<bool_t>();
    if (has_form && *has_form) {
        (void)wrapper.read<int_le>(); // strip FormId
    }
}

// Stubs for complex handlers
void rewrite_start_game(PacketWrapper& wrapper) {
    wrapper.passthrough_all();
}

void rewrite_camera_instruction(PacketWrapper& wrapper) {
    wrapper.passthrough_all();
}

void rewrite_camera_spline(PacketWrapper& wrapper) {
    wrapper.passthrough_all();
}

} // namespace endweave::v944_to_v924
