#include "endweave/protocols/v2168/camera.h"

#include <optional>
#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::CameraPreset_<2168>, bp::CameraPreset_<2181>>::transform(Context<bp::CameraPreset_<2181>> &ctx,
                                                                              bp::CameraPreset_<2168> &&from)
{
    auto &to = ctx.out();
    to.name = std::move(from.name);
    to.inherit_from = std::move(from.inherit_from);
    to.pos_x = from.pos_x;
    to.pos_y = from.pos_y;
    to.pos_z = from.pos_z;
    to.rot_x = from.rot_x;
    to.rot_y = from.rot_y;
    to.camera_rotation_speed = from.camera_rotation_speed;
    to.snap_to_target = from.snap_to_target;
    to.horizontal_rotation_limit = from.horizontal_rotation_limit;
    to.vertical_rotation_limit = from.vertical_rotation_limit;
    to.continue_targeting = from.continue_targeting;
    to.tracking_radius = from.tracking_radius;
    to.view_offset = from.view_offset;
    to.entity_offset = from.entity_offset;
    to.radius = from.radius;
    to.yaw_limit_min = from.yaw_limit_min;
    to.yaw_limit_max = from.yaw_limit_max;
    to.listener = from.listener;
    to.player_effects = from.player_effects;
    to.aim_assist = std::move(from.aim_assist);
    to.control_scheme = from.control_scheme;
    // ENDWEAVE: 2168 has no starting rotation to inherit or to send, and false with an absent pair is
    // what 2181 writes for a preset that sets neither.
    to.apply_inherited_starting_rotation = false;
    to.starting_rotation = std::nullopt;
}

void Transformer<bp::CameraPresets_<2168>, bp::CameraPresets_<2181>>::transform(Context<bp::CameraPresets_<2181>> &ctx,
                                                                                bp::CameraPresets_<2168> &&from)
{
    auto &to = ctx.out();
    to.presets = ew::transform(ctx, std::move(from.presets));
}

void Transformer<bp::CameraPresetsPacket_<2168>, bp::CameraPresetsPacket_<2181>>::transform(
    Context<bp::CameraPresetsPacket_<2181>> &ctx, bp::CameraPresetsPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.camera_presets = ew::transform(ctx, std::move(from.camera_presets));
}

} // namespace endweave
