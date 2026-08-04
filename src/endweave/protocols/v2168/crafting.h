#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/crafting.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::SerializedNetworkItemInstanceDescriptor_<2168>> {
    static bp::SerializedNetworkItemInstanceDescriptor_<1001> downgrade(
        bp::SerializedNetworkItemInstanceDescriptor_<2168> &&from);
};

template <>
struct Transformer<bp::SerializedRecipeIngredient_<2168>> {
    static bp::SerializedRecipeIngredient_<1001> downgrade(bp::SerializedRecipeIngredient_<2168> &&from);
};

template <>
struct Transformer<bp::SerializedRecipeUnlockingRequirement_<2168>> {
    static bp::SerializedRecipeUnlockingRequirement_<1001> downgrade(
        bp::SerializedRecipeUnlockingRequirement_<2168> &&from);
};

template <>
struct Transformer<bp::ShapedRecipePayload_<2168>> {
    static bp::ShapedRecipePayload_<1001> downgrade(bp::ShapedRecipePayload_<2168> &&from);
};

template <>
struct Transformer<bp::ShapelessRecipePayload_<2168>> {
    static bp::ShapelessRecipePayload_<1001> downgrade(bp::ShapelessRecipePayload_<2168> &&from);
};

template <>
struct Transformer<bp::SmithingTransformRecipePayload_<2168>> {
    static bp::SmithingTransformRecipePayload_<1001> downgrade(bp::SmithingTransformRecipePayload_<2168> &&from);
};

template <>
struct Transformer<bp::SmithingTrimRecipePayload_<2168>> {
    static bp::SmithingTrimRecipePayload_<1001> downgrade(bp::SmithingTrimRecipePayload_<2168> &&from);
};

template <>
struct Transformer<bp::CraftingDataPacket_<2168>> {
    static bp::CraftingDataPacket_<1001> downgrade(bp::CraftingDataPacket_<2168> &&from);
};

template <>
struct Transformer<bp::CreativeGroupInfoPayload_<2168>> {
    static bp::CreativeGroupInfoPayload_<1001> downgrade(bp::CreativeGroupInfoPayload_<2168> &&from);
};

template <>
struct Transformer<bp::CreativeItemEntryPayload_<2168>> {
    static bp::CreativeItemEntryPayload_<1001> downgrade(bp::CreativeItemEntryPayload_<2168> &&from);
};

template <>
struct Transformer<bp::CreativeContentPacket_<2168>> {
    static bp::CreativeContentPacket_<1001> downgrade(bp::CreativeContentPacket_<2168> &&from);
};

} // namespace endweave
