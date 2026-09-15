#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/camera.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::CameraPreset_<2168>, bp::CameraPreset_<2193>> {
    static void transform(Context<bp::CameraPreset_<2193>> &ctx, bp::CameraPreset_<2168> &&from);
};
} // namespace endweave
