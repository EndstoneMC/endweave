#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/skin.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::SerializedSkinRef_<1001>> {
    static bp::SerializedSkinRef_<2168> upgrade(bp::SerializedSkinRef_<1001> &&from);
};

template <>
struct Transformer<bp::PlayerSkinPacket_<1001>> {
    static bp::PlayerSkinPacket_<2168> upgrade(bp::PlayerSkinPacket_<1001> &&from);
};

} // namespace endweave
