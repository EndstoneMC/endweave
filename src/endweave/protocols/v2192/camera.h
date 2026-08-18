#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/camera.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::CameraPreset_<2192>, bp::CameraPreset_<2168>> {
    static void transform(Context<bp::CameraPreset_<2168>> &ctx, bp::CameraPreset_<2192> &&from);
};

template <>
struct Transformer<bp::CameraPresets_<2192>, bp::CameraPresets_<2168>> {
    static void transform(Context<bp::CameraPresets_<2168>> &ctx, bp::CameraPresets_<2192> &&from);
};

template <>
struct Transformer<bp::CameraPresetsPacket_<2192>, bp::CameraPresetsPacket_<2168>> {
    static void transform(Context<bp::CameraPresetsPacket_<2168>> &ctx, bp::CameraPresetsPacket_<2192> &&from);
};

} // namespace endweave
