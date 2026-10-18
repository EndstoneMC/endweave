#pragma once

#include "endweave/protocol/transform.h"
#include "endweave/protocols/enums.h"

#include <bedrock/protocol/skin.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::SerializedSkinRef_<2193>, bp::SerializedSkinRef_<2225>> {
    static void transform(Context<bp::SerializedSkinRef_<2225>> &ctx, bp::SerializedSkinRef_<2193> &&from);
};
} // namespace endweave
