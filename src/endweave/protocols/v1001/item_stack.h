#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/item_stack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::RedactableString_<1001>, bp::RedactableString_<2168>> {
    static void transform(Context<bp::RedactableString_<2168>> &ctx, bp::RedactableString_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseSlotInfo_<1001>, bp::ItemStackResponseSlotInfo_<2168>> {
    static void transform(Context<bp::ItemStackResponseSlotInfo_<2168>> &ctx,
                          bp::ItemStackResponseSlotInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseInfo_<1001>, bp::ItemStackResponseInfo_<2168>> {
    static void transform(Context<bp::ItemStackResponseInfo_<2168>> &ctx, bp::ItemStackResponseInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestSlotInfo_<1001>, bp::ItemStackRequestCereal_<2168>::SlotInfoData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::SlotInfoData> &ctx,
                          bp::ItemStackRequestSlotInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionTake_<1001>, bp::ItemStackRequestCereal_<2168>::TakeActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::TakeActionData> &ctx,
                          bp::ItemStackRequestActionTake_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionPlace_<1001>, bp::ItemStackRequestCereal_<2168>::PlaceActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::PlaceActionData> &ctx,
                          bp::ItemStackRequestActionPlace_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionSwap_<1001>, bp::ItemStackRequestCereal_<2168>::SwapActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::SwapActionData> &ctx,
                          bp::ItemStackRequestActionSwap_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionDrop_<1001>, bp::ItemStackRequestCereal_<2168>::DropActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::DropActionData> &ctx,
                          bp::ItemStackRequestActionDrop_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionDestroy_<1001>, bp::ItemStackRequestCereal_<2168>::DestroyActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::DestroyActionData> &ctx,
                          bp::ItemStackRequestActionDestroy_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionConsume_<1001>, bp::ItemStackRequestCereal_<2168>::ConsumeActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::ConsumeActionData> &ctx,
                          bp::ItemStackRequestActionConsume_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCreate_<1001>, bp::ItemStackRequestCereal_<2168>::CreateActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::CreateActionData> &ctx,
                          bp::ItemStackRequestActionCreate_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionLabTableCombine_<1001>,
                   bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData> &ctx,
                          bp::ItemStackRequestActionLabTableCombine_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionBeaconPayment_<1001>,
                   bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData> &ctx,
                          bp::ItemStackRequestActionBeaconPayment_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionMineBlock_<1001>, bp::ItemStackRequestCereal_<2168>::MineBlockActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::MineBlockActionData> &ctx,
                          bp::ItemStackRequestActionMineBlock_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftRecipe_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData> &ctx,
                          bp::ItemStackRequestActionCraftRecipe_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftRecipeAuto_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData> &ctx,
                          bp::ItemStackRequestActionCraftRecipeAuto_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftCreative_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData> &ctx,
                          bp::ItemStackRequestActionCraftCreative_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftRecipeOptional_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData> &ctx,
                          bp::ItemStackRequestActionCraftRecipeOptional_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftGrindstone_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData> &ctx,
                          bp::ItemStackRequestActionCraftGrindstone_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftLoom_<1001>, bp::ItemStackRequestCereal_<2168>::CraftLoomActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::CraftLoomActionData> &ctx,
                          bp::ItemStackRequestActionCraftLoom_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData> &ctx,
                          bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftResultsActionData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::CraftResultsActionData> &ctx,
                          bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestData_<1001>, bp::ItemStackRequestCereal_<2168>::RequestData> {
    static void transform(Context<bp::ItemStackRequestCereal_<2168>::RequestData> &ctx,
                          bp::ItemStackRequestData_<1001> &&from);
};

} // namespace endweave
