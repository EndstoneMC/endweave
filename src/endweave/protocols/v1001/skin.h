#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/skin.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::legacy::AnimatedImageData, bp::AnimatedImageData> {
    static bp::AnimatedImageData transform(bp::legacy::AnimatedImageData &&from);
};

template <>
struct Transformer<bp::AnimatedImageData, bp::legacy::AnimatedImageData> {
    static bp::legacy::AnimatedImageData transform(bp::AnimatedImageData &&from);
};

template <>
struct Transformer<bp::legacy::SerializedPersonaPieceHandle, bp::SerializedPersonaPieceHandle> {
    static bp::SerializedPersonaPieceHandle transform(bp::legacy::SerializedPersonaPieceHandle &&from);
};

template <>
struct Transformer<bp::SerializedPersonaPieceHandle, bp::legacy::SerializedPersonaPieceHandle> {
    static bp::legacy::SerializedPersonaPieceHandle transform(bp::SerializedPersonaPieceHandle &&from);
};

template <>
struct Transformer<bp::legacy::TintMapColor, bp::TintMapColor> {
    static bp::TintMapColor transform(bp::legacy::TintMapColor &&from);
};

template <>
struct Transformer<bp::TintMapColor, bp::legacy::TintMapColor> {
    static bp::legacy::TintMapColor transform(bp::TintMapColor &&from);
};

template <>
struct Transformer<bp::legacy::SerializedSkinRef, bp::SerializedSkinRef_<1001>> {
    static bp::SerializedSkinRef_<1001> transform(bp::legacy::SerializedSkinRef &&from);
};

template <>
struct Transformer<bp::SerializedSkinRef_<1001>, bp::SerializedSkinRef_<2168>> {
    static bp::SerializedSkinRef_<2168> transform(bp::SerializedSkinRef_<1001> &&from);
};

template <>
struct Transformer<bp::SerializedSkinRef_<1001>, bp::legacy::SerializedSkinRef> {
    static bp::legacy::SerializedSkinRef transform(bp::SerializedSkinRef_<1001> &&from);
};

template <>
struct Transformer<bp::PlayerSkinPacket_<1001>, bp::PlayerSkinPacket_<2168>> {
    static bp::PlayerSkinPacket_<2168> transform(bp::PlayerSkinPacket_<1001> &&from);
};

} // namespace endweave
