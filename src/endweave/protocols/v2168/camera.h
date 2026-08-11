#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/camera.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::CameraPreset_<2168>, bp::CameraPreset_<2187>> {
    static void transform(Context<bp::CameraPreset_<2187>> &ctx, bp::CameraPreset_<2168> &&from);
};

template <>
struct Transformer<bp::CameraPresets_<2168>, bp::CameraPresets_<2187>> {
    static void transform(Context<bp::CameraPresets_<2187>> &ctx, bp::CameraPresets_<2168> &&from);
};

template <>
struct Transformer<bp::CameraPresetsPacket_<2168>, bp::CameraPresetsPacket_<2187>> {
    static void transform(Context<bp::CameraPresetsPacket_<2187>> &ctx, bp::CameraPresetsPacket_<2168> &&from);
};

} // namespace endweave
