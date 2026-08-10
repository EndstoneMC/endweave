#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/item_stack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::RedactableString_<2168>, bp::RedactableString_<1001>> {
    static void transform(Context<bp::RedactableString_<1001>> &ctx, bp::RedactableString_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseSlotInfo_<2168>, bp::ItemStackResponseSlotInfo_<1001>> {
    static void transform(Context<bp::ItemStackResponseSlotInfo_<1001>> &ctx,
                          bp::ItemStackResponseSlotInfo_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseContainerInfo_<2168>, bp::ItemStackResponseContainerInfo_<1001>> {
    static void transform(Context<bp::ItemStackResponseContainerInfo_<1001>> &ctx,
                          bp::ItemStackResponseContainerInfo_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseInfo_<2168>, bp::ItemStackResponseInfo_<1001>> {
    static void transform(Context<bp::ItemStackResponseInfo_<1001>> &ctx, bp::ItemStackResponseInfo_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackResponsePacket_<2168>, bp::ItemStackResponsePacket_<1001>> {
    static void transform(Context<bp::ItemStackResponsePacket_<1001>> &ctx, bp::ItemStackResponsePacket_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::SlotInfoData, bp::ItemStackRequestSlotInfo_<1001>> {
    static void transform(Context<bp::ItemStackRequestSlotInfo_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::SlotInfoData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::TakeActionData, bp::ItemStackRequestActionTake_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionTake_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::TakeActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::PlaceActionData, bp::ItemStackRequestActionPlace_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionPlace_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::PlaceActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::SwapActionData, bp::ItemStackRequestActionSwap_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionSwap_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::SwapActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::DropActionData, bp::ItemStackRequestActionDrop_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionDrop_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::DropActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::DestroyActionData, bp::ItemStackRequestActionDestroy_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionDestroy_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::DestroyActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::ConsumeActionData, bp::ItemStackRequestActionConsume_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionConsume_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::ConsumeActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CreateActionData, bp::ItemStackRequestActionCreate_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionCreate_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::CreateActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData,
                   bp::ItemStackRequestActionLabTableCombine_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionLabTableCombine_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData,
                   bp::ItemStackRequestActionBeaconPayment_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionBeaconPayment_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::MineBlockActionData, bp::ItemStackRequestActionMineBlock_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionMineBlock_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::MineBlockActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData,
                   bp::ItemStackRequestActionCraftRecipe_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionCraftRecipe_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData,
                   bp::ItemStackRequestActionCraftRecipeAuto_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionCraftRecipeAuto_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData,
                   bp::ItemStackRequestActionCraftCreative_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionCraftCreative_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData,
                   bp::ItemStackRequestActionCraftRecipeOptional_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionCraftRecipeOptional_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData,
                   bp::ItemStackRequestActionCraftGrindstone_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionCraftGrindstone_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftLoomActionData, bp::ItemStackRequestActionCraftLoom_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionCraftLoom_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::CraftLoomActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData,
                   bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftResultsActionData,
                   bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001>> {
    static void transform(Context<bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::CraftResultsActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::RequestData, bp::ItemStackRequestData_<1001>> {
    static void transform(Context<bp::ItemStackRequestData_<1001>> &ctx,
                          bp::ItemStackRequestCereal_<2168>::RequestData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestPacket_<2168>, bp::ItemStackRequestPacket_<1001>> {
    static void transform(Context<bp::ItemStackRequestPacket_<1001>> &ctx, bp::ItemStackRequestPacket_<2168> &&from);
};

} // namespace endweave
