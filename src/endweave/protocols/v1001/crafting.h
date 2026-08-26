#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/crafting.h>

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
struct Transformer<bp::CraftingDataPacket_<1001>, bp::CraftingDataPacket_<2168>> {
    static void transform(Context<bp::CraftingDataPacket_<2168>> &ctx, bp::CraftingDataPacket_<1001> &&from);
};

} // namespace endweave
