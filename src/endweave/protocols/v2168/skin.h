#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/skin.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::SerializedSkinRef_<2168>, bp::SerializedSkinRef_<1001>> {
    static bp::SerializedSkinRef_<1001> transform(bp::SerializedSkinRef_<2168> &&from);
};

template <>
struct Transformer<bp::PlayerSkinPacket_<2168>, bp::PlayerSkinPacket_<1001>> {
    static bp::PlayerSkinPacket_<1001> transform(bp::PlayerSkinPacket_<2168> &&from);
};

} // namespace endweave
