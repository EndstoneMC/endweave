#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/skin.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::legacy::AnimatedImageData> {
    static bp::AnimatedImageData toCereal(bp::legacy::AnimatedImageData &&from);
};

template <>
struct Transformer<bp::AnimatedImageData> {
    static bp::legacy::AnimatedImageData toLegacy(bp::AnimatedImageData &&from);
};

template <>
struct Transformer<bp::legacy::SerializedPersonaPieceHandle> {
    static bp::SerializedPersonaPieceHandle toCereal(bp::legacy::SerializedPersonaPieceHandle &&from);
};

template <>
struct Transformer<bp::SerializedPersonaPieceHandle> {
    static bp::legacy::SerializedPersonaPieceHandle toLegacy(bp::SerializedPersonaPieceHandle &&from);
};

template <>
struct Transformer<bp::legacy::TintMapColor> {
    static bp::TintMapColor toCereal(bp::legacy::TintMapColor &&from);
};

template <>
struct Transformer<bp::TintMapColor> {
    static bp::legacy::TintMapColor toLegacy(bp::TintMapColor &&from);
};

template <>
struct Transformer<bp::legacy::SerializedSkinRef> {
    static bp::SerializedSkinRef_<1001> toCereal(bp::legacy::SerializedSkinRef &&from);
};

template <>
struct Transformer<bp::SerializedSkinRef_<1001>> {
    static bp::SerializedSkinRef_<2168> upgrade(bp::SerializedSkinRef_<1001> &&from);
    static bp::legacy::SerializedSkinRef toLegacy(bp::SerializedSkinRef_<1001> &&from);
};

template <>
struct Transformer<bp::PlayerSkinPacket_<1001>> {
    static bp::PlayerSkinPacket_<2168> upgrade(bp::PlayerSkinPacket_<1001> &&from);
};

} // namespace endweave
