#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/item_stack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::RedactableString_<1001>> {
    static bp::RedactableString_<2168> upgrade(bp::RedactableString_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseSlotInfo_<1001>> {
    static bp::ItemStackResponseSlotInfo_<2168> upgrade(bp::ItemStackResponseSlotInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseContainerInfo_<1001>> {
    static bp::ItemStackResponseContainerInfo_<2168> upgrade(bp::ItemStackResponseContainerInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseInfo_<1001>> {
    static bp::ItemStackResponseInfo_<2168> upgrade(bp::ItemStackResponseInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackResponsePacket_<1001>> {
    static bp::ItemStackResponsePacket_<2168> upgrade(bp::ItemStackResponsePacket_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestSlotInfo_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::SlotInfoData upgrade(bp::ItemStackRequestSlotInfo_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionTake_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::TakeActionData upgrade(bp::ItemStackRequestActionTake_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionPlace_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::PlaceActionData upgrade(bp::ItemStackRequestActionPlace_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionSwap_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::SwapActionData upgrade(bp::ItemStackRequestActionSwap_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionDrop_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::DropActionData upgrade(bp::ItemStackRequestActionDrop_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionDestroy_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::DestroyActionData upgrade(
        bp::ItemStackRequestActionDestroy_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionConsume_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::ConsumeActionData upgrade(
        bp::ItemStackRequestActionConsume_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCreate_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::CreateActionData upgrade(bp::ItemStackRequestActionCreate_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionLabTableCombine_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData upgrade(
        bp::ItemStackRequestActionLabTableCombine_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionBeaconPayment_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData upgrade(
        bp::ItemStackRequestActionBeaconPayment_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionMineBlock_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::MineBlockActionData upgrade(
        bp::ItemStackRequestActionMineBlock_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftRecipe_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData upgrade(
        bp::ItemStackRequestActionCraftRecipe_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftRecipeAuto_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData upgrade(
        bp::ItemStackRequestActionCraftRecipeAuto_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftCreative_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData upgrade(
        bp::ItemStackRequestActionCraftCreative_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftRecipeOptional_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData upgrade(
        bp::ItemStackRequestActionCraftRecipeOptional_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftGrindstone_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData upgrade(
        bp::ItemStackRequestActionCraftGrindstone_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftLoom_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::CraftLoomActionData upgrade(
        bp::ItemStackRequestActionCraftLoom_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData upgrade(
        bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::CraftResultsActionData upgrade(
        bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestData_<1001>> {
    static bp::ItemStackRequestCereal_<2168>::RequestData upgrade(bp::ItemStackRequestData_<1001> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestPacket_<1001>> {
    static bp::ItemStackRequestPacket_<2168> upgrade(bp::ItemStackRequestPacket_<1001> &&from);
};

} // namespace endweave
