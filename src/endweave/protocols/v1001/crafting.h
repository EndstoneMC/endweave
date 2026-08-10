#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/crafting.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::SerializedNetworkItemInstanceDescriptor_<1001>,
                   bp::SerializedNetworkItemInstanceDescriptor_<2168>> {
    static void transform(Context<bp::SerializedNetworkItemInstanceDescriptor_<2168>> &ctx,
                          bp::SerializedNetworkItemInstanceDescriptor_<1001> &&from);
};

template <>
struct Transformer<bp::SerializedRecipeIngredient_<1001>, bp::SerializedRecipeIngredient_<2168>> {
    static void transform(Context<bp::SerializedRecipeIngredient_<2168>> &ctx,
                          bp::SerializedRecipeIngredient_<1001> &&from);
};

template <>
struct Transformer<bp::SerializedRecipeUnlockingRequirement_<1001>, bp::SerializedRecipeUnlockingRequirement_<2168>> {
    static void transform(Context<bp::SerializedRecipeUnlockingRequirement_<2168>> &ctx,
                          bp::SerializedRecipeUnlockingRequirement_<1001> &&from);
};

template <>
struct Transformer<bp::ShapedRecipePayload_<1001>, bp::ShapedRecipePayload_<2168>> {
    static void transform(Context<bp::ShapedRecipePayload_<2168>> &ctx, bp::ShapedRecipePayload_<1001> &&from);
};

template <>
struct Transformer<bp::ShapelessRecipePayload_<1001>, bp::ShapelessRecipePayload_<2168>> {
    static void transform(Context<bp::ShapelessRecipePayload_<2168>> &ctx, bp::ShapelessRecipePayload_<1001> &&from);
};

template <>
struct Transformer<bp::SmithingTransformRecipePayload_<1001>, bp::SmithingTransformRecipePayload_<2168>> {
    static void transform(Context<bp::SmithingTransformRecipePayload_<2168>> &ctx,
                          bp::SmithingTransformRecipePayload_<1001> &&from);
};

template <>
struct Transformer<bp::SmithingTrimRecipePayload_<1001>, bp::SmithingTrimRecipePayload_<2168>> {
    static void transform(Context<bp::SmithingTrimRecipePayload_<2168>> &ctx,
                          bp::SmithingTrimRecipePayload_<1001> &&from);
};

template <>
struct Transformer<bp::CraftingDataPacket_<1001>, bp::CraftingDataPacket_<2168>> {
    static void transform(Context<bp::CraftingDataPacket_<2168>> &ctx, bp::CraftingDataPacket_<1001> &&from);
};

template <>
struct Transformer<bp::CreativeGroupInfoPayload_<1001>, bp::CreativeGroupInfoPayload_<2168>> {
    static void transform(Context<bp::CreativeGroupInfoPayload_<2168>> &ctx,
                          bp::CreativeGroupInfoPayload_<1001> &&from);
};

template <>
struct Transformer<bp::CreativeItemEntryPayload_<1001>, bp::CreativeItemEntryPayload_<2168>> {
    static void transform(Context<bp::CreativeItemEntryPayload_<2168>> &ctx,
                          bp::CreativeItemEntryPayload_<1001> &&from);
};

template <>
struct Transformer<bp::CreativeContentPacket_<1001>, bp::CreativeContentPacket_<2168>> {
    static void transform(Context<bp::CreativeContentPacket_<2168>> &ctx, bp::CreativeContentPacket_<1001> &&from);
};

} // namespace endweave
