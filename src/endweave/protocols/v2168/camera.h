#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/camera.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::CameraPreset_<2168>, bp::CameraPreset_<2181>> {
    static bp::CameraPreset_<2181> transform(bp::CameraPreset_<2168> &&from);
};

template <>
struct Transformer<bp::CameraPresets_<2168>, bp::CameraPresets_<2181>> {
    static bp::CameraPresets_<2181> transform(bp::CameraPresets_<2168> &&from);
};

template <>
struct Transformer<bp::CameraPresetsPacket_<2168>, bp::CameraPresetsPacket_<2181>> {
    static bp::CameraPresetsPacket_<2181> transform(bp::CameraPresetsPacket_<2168> &&from);
};

} // namespace endweave
