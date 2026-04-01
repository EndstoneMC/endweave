/// v924 -> v944 miscellaneous handlers (voxel_shapes, data_driven_ui, start_game stubs).

#include "endweave/protocol/v924_to_v944/handlers.h"

#include "endweave/codec/types/primitives.h"

namespace endweave::v924_to_v944 {

void rewrite_voxel_shapes(PacketWrapper& wrapper) {
    // Append Custom Shape Count (new in v944, set to 0)
    wrapper.passthrough_all();
    wrapper.write<uvar_int>(0); // Custom Shape Count
}

void rewrite_show_screen(PacketWrapper& wrapper) {
    // Pass through v924 fields, append missing v944 fields
    wrapper.passthrough_all();
    wrapper.write<var_int>(0);  // FormId
    wrapper.write<string>(std::string("")); // DataInstanceId
}

void rewrite_close_all_screens(PacketWrapper& wrapper) {
    // v924 CloseAllScreens -> v944 CloseScreen
    // v924 has no fields, v944 adds optional FormId
    wrapper.write<bool_t>(false); // has FormId
}

// Stubs for complex handlers that need LevelSettings/Camera types
// These will be fully implemented when those compound types are ported

void rewrite_start_game(PacketWrapper& wrapper) {
    // TODO: implement when LevelSettings compound type is ported
    wrapper.passthrough_all();
}

void rewrite_camera_instruction(PacketWrapper& wrapper) {
    // TODO: implement when SplineInstruction compound type is ported
    wrapper.passthrough_all();
}

void rewrite_camera_spline(PacketWrapper& wrapper) {
    // TODO: implement when SplineInstruction compound type is ported
    wrapper.passthrough_all();
}

} // namespace endweave::v924_to_v944
