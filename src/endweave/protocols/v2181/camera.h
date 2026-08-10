#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/camera.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::CameraPreset_<2181>, bp::CameraPreset_<2168>> {
    static void transform(Context<bp::CameraPreset_<2168>> &ctx, bp::CameraPreset_<2181> &&from);
};

template <>
struct Transformer<bp::CameraPresets_<2181>, bp::CameraPresets_<2168>> {
    static void transform(Context<bp::CameraPresets_<2168>> &ctx, bp::CameraPresets_<2181> &&from);
};

template <>
struct Transformer<bp::CameraPresetsPacket_<2181>, bp::CameraPresetsPacket_<2168>> {
    static void transform(Context<bp::CameraPresetsPacket_<2168>> &ctx, bp::CameraPresetsPacket_<2181> &&from);
};

} // namespace endweave
