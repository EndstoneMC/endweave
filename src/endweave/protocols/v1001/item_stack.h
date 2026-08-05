#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/item_stack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::RedactableString_<1001>, bp::RedactableString_<2168>> {
    static bp::RedactableString_<2168> transform(bp::RedactableString_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseSlotInfo_<1001>, bp::ItemStackResponseSlotInfo_<2168>> {
    static bp::ItemStackResponseSlotInfo_<2168> transform(bp::ItemStackResponseSlotInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseContainerInfo_<1001>, bp::ItemStackResponseContainerInfo_<2168>> {
    static bp::ItemStackResponseContainerInfo_<2168> transform(bp::ItemStackResponseContainerInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseInfo_<1001>, bp::ItemStackResponseInfo_<2168>> {
    static bp::ItemStackResponseInfo_<2168> transform(bp::ItemStackResponseInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponsePacket_<1001>, bp::ItemStackResponsePacket_<2168>> {
    static bp::ItemStackResponsePacket_<2168> transform(bp::ItemStackResponsePacket_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestSlotInfo_<1001>, bp::ItemStackRequestCereal_<2168>::SlotInfoData> {
    static bp::ItemStackRequestCereal_<2168>::SlotInfoData transform(bp::ItemStackRequestSlotInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionTake_<1001>, bp::ItemStackRequestCereal_<2168>::TakeActionData> {
    static bp::ItemStackRequestCereal_<2168>::TakeActionData transform(bp::ItemStackRequestActionTake_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionPlace_<1001>, bp::ItemStackRequestCereal_<2168>::PlaceActionData> {
    static bp::ItemStackRequestCereal_<2168>::PlaceActionData transform(bp::ItemStackRequestActionPlace_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionSwap_<1001>, bp::ItemStackRequestCereal_<2168>::SwapActionData> {
    static bp::ItemStackRequestCereal_<2168>::SwapActionData transform(bp::ItemStackRequestActionSwap_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionDrop_<1001>, bp::ItemStackRequestCereal_<2168>::DropActionData> {
    static bp::ItemStackRequestCereal_<2168>::DropActionData transform(bp::ItemStackRequestActionDrop_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionDestroy_<1001>, bp::ItemStackRequestCereal_<2168>::DestroyActionData> {
    static bp::ItemStackRequestCereal_<2168>::DestroyActionData transform(
        bp::ItemStackRequestActionDestroy_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionConsume_<1001>, bp::ItemStackRequestCereal_<2168>::ConsumeActionData> {
    static bp::ItemStackRequestCereal_<2168>::ConsumeActionData transform(
        bp::ItemStackRequestActionConsume_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCreate_<1001>, bp::ItemStackRequestCereal_<2168>::CreateActionData> {
    static bp::ItemStackRequestCereal_<2168>::CreateActionData transform(
        bp::ItemStackRequestActionCreate_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionLabTableCombine_<1001>,
                   bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData> {
    static bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData transform(
        bp::ItemStackRequestActionLabTableCombine_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionBeaconPayment_<1001>,
                   bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData> {
    static bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData transform(
        bp::ItemStackRequestActionBeaconPayment_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionMineBlock_<1001>, bp::ItemStackRequestCereal_<2168>::MineBlockActionData> {
    static bp::ItemStackRequestCereal_<2168>::MineBlockActionData transform(
        bp::ItemStackRequestActionMineBlock_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftRecipe_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData> {
    static bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData transform(
        bp::ItemStackRequestActionCraftRecipe_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftRecipeAuto_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData> {
    static bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData transform(
        bp::ItemStackRequestActionCraftRecipeAuto_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftCreative_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData> {
    static bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData transform(
        bp::ItemStackRequestActionCraftCreative_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftRecipeOptional_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData> {
    static bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData transform(
        bp::ItemStackRequestActionCraftRecipeOptional_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftGrindstone_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData> {
    static bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData transform(
        bp::ItemStackRequestActionCraftGrindstone_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftLoom_<1001>, bp::ItemStackRequestCereal_<2168>::CraftLoomActionData> {
    static bp::ItemStackRequestCereal_<2168>::CraftLoomActionData transform(
        bp::ItemStackRequestActionCraftLoom_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData> {
    static bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData transform(
        bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001>,
                   bp::ItemStackRequestCereal_<2168>::CraftResultsActionData> {
    static bp::ItemStackRequestCereal_<2168>::CraftResultsActionData transform(
        bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestData_<1001>, bp::ItemStackRequestCereal_<2168>::RequestData> {
    static bp::ItemStackRequestCereal_<2168>::RequestData transform(bp::ItemStackRequestData_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestPacket_<1001>, bp::ItemStackRequestPacket_<2168>> {
    static bp::ItemStackRequestPacket_<2168> transform(bp::ItemStackRequestPacket_<1001> &&from);
};

} // namespace endweave
