#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/skin.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::legacy::AnimatedImageData, bp::AnimatedImageData> {
    static void transform(Context<bp::AnimatedImageData> &ctx, bp::legacy::AnimatedImageData &&from);
};

template <>
struct Transformer<bp::AnimatedImageData, bp::legacy::AnimatedImageData> {
    static void transform(Context<bp::legacy::AnimatedImageData> &ctx, bp::AnimatedImageData &&from);
};

template <>
struct Transformer<bp::legacy::SerializedPersonaPieceHandle, bp::SerializedPersonaPieceHandle> {
    static void transform(Context<bp::SerializedPersonaPieceHandle> &ctx,
                          bp::legacy::SerializedPersonaPieceHandle &&from);
};

template <>
struct Transformer<bp::SerializedPersonaPieceHandle, bp::legacy::SerializedPersonaPieceHandle> {
    static void transform(Context<bp::legacy::SerializedPersonaPieceHandle> &ctx,
                          bp::SerializedPersonaPieceHandle &&from);
};

template <>
struct Transformer<bp::legacy::TintMapColor, bp::TintMapColor> {
    static void transform(Context<bp::TintMapColor> &ctx, bp::legacy::TintMapColor &&from);
};

template <>
struct Transformer<bp::TintMapColor, bp::legacy::TintMapColor> {
    static void transform(Context<bp::legacy::TintMapColor> &ctx, bp::TintMapColor &&from);
};

template <>
struct Transformer<bp::legacy::SerializedSkinRef, bp::SerializedSkinRef_<1001>> {
    static void transform(Context<bp::SerializedSkinRef_<1001>> &ctx, bp::legacy::SerializedSkinRef &&from);
};

template <>
struct Transformer<bp::SerializedSkinRef_<1001>, bp::SerializedSkinRef_<2168>> {
    static void transform(Context<bp::SerializedSkinRef_<2168>> &ctx, bp::SerializedSkinRef_<1001> &&from);
};

template <>
struct Transformer<bp::SerializedSkinRef_<1001>, bp::legacy::SerializedSkinRef> {
    static void transform(Context<bp::legacy::SerializedSkinRef> &ctx, bp::SerializedSkinRef_<1001> &&from);
};

} // namespace endweave
