#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/crafting.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::SerializedNetworkItemInstanceDescriptor_<1001>> {
    static bp::SerializedNetworkItemInstanceDescriptor_<2168> upgrade(
        bp::SerializedNetworkItemInstanceDescriptor_<1001> &&from);
};

template <>
struct Transformer<bp::SerializedRecipeIngredient_<1001>> {
    static bp::SerializedRecipeIngredient_<2168> upgrade(bp::SerializedRecipeIngredient_<1001> &&from);
};

template <>
struct Transformer<bp::SerializedRecipeUnlockingRequirement_<1001>> {
    static bp::SerializedRecipeUnlockingRequirement_<2168> upgrade(
        bp::SerializedRecipeUnlockingRequirement_<1001> &&from);
};

template <>
struct Transformer<bp::ShapedRecipePayload_<1001>> {
    static bp::ShapedRecipePayload_<2168> upgrade(bp::ShapedRecipePayload_<1001> &&from);
};

template <>
struct Transformer<bp::ShapelessRecipePayload_<1001>> {
    static bp::ShapelessRecipePayload_<2168> upgrade(bp::ShapelessRecipePayload_<1001> &&from);
};

template <>
struct Transformer<bp::SmithingTransformRecipePayload_<1001>> {
    static bp::SmithingTransformRecipePayload_<2168> upgrade(bp::SmithingTransformRecipePayload_<1001> &&from);
};

template <>
struct Transformer<bp::SmithingTrimRecipePayload_<1001>> {
    static bp::SmithingTrimRecipePayload_<2168> upgrade(bp::SmithingTrimRecipePayload_<1001> &&from);
};

template <>
struct Transformer<bp::CraftingDataPacket_<1001>> {
    static bp::CraftingDataPacket_<2168> upgrade(bp::CraftingDataPacket_<1001> &&from);
};

template <>
struct Transformer<bp::CreativeGroupInfoPayload_<1001>> {
    static bp::CreativeGroupInfoPayload_<2168> upgrade(bp::CreativeGroupInfoPayload_<1001> &&from);
};

template <>
struct Transformer<bp::CreativeItemEntryPayload_<1001>> {
    static bp::CreativeItemEntryPayload_<2168> upgrade(bp::CreativeItemEntryPayload_<1001> &&from);
};

template <>
struct Transformer<bp::CreativeContentPacket_<1001>> {
    static bp::CreativeContentPacket_<2168> upgrade(bp::CreativeContentPacket_<1001> &&from);
};

} // namespace endweave
