#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/item_stack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::RedactableString_<2168>> {
    static bp::RedactableString_<1001> downgrade(bp::RedactableString_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseSlotInfo_<2168>> {
    static bp::ItemStackResponseSlotInfo_<1001> downgrade(bp::ItemStackResponseSlotInfo_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseContainerInfo_<2168>> {
    static bp::ItemStackResponseContainerInfo_<1001> downgrade(bp::ItemStackResponseContainerInfo_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseInfo_<2168>> {
    static bp::ItemStackResponseInfo_<1001> downgrade(bp::ItemStackResponseInfo_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackResponsePacket_<2168>> {
    static bp::ItemStackResponsePacket_<1001> downgrade(bp::ItemStackResponsePacket_<2168> &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::SlotInfoData> {
    static bp::ItemStackRequestSlotInfo_<1001> downgrade(bp::ItemStackRequestCereal_<2168>::SlotInfoData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::TakeActionData> {
    static bp::ItemStackRequestActionTake_<1001> downgrade(bp::ItemStackRequestCereal_<2168>::TakeActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::PlaceActionData> {
    static bp::ItemStackRequestActionPlace_<1001> downgrade(bp::ItemStackRequestCereal_<2168>::PlaceActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::SwapActionData> {
    static bp::ItemStackRequestActionSwap_<1001> downgrade(bp::ItemStackRequestCereal_<2168>::SwapActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::DropActionData> {
    static bp::ItemStackRequestActionDrop_<1001> downgrade(bp::ItemStackRequestCereal_<2168>::DropActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::DestroyActionData> {
    static bp::ItemStackRequestActionDestroy_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::DestroyActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::ConsumeActionData> {
    static bp::ItemStackRequestActionConsume_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::ConsumeActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CreateActionData> {
    static bp::ItemStackRequestActionCreate_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::CreateActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData> {
    static bp::ItemStackRequestActionLabTableCombine_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData> {
    static bp::ItemStackRequestActionBeaconPayment_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::MineBlockActionData> {
    static bp::ItemStackRequestActionMineBlock_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::MineBlockActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData> {
    static bp::ItemStackRequestActionCraftRecipe_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData> {
    static bp::ItemStackRequestActionCraftRecipeAuto_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData> {
    static bp::ItemStackRequestActionCraftCreative_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData> {
    static bp::ItemStackRequestActionCraftRecipeOptional_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData> {
    static bp::ItemStackRequestActionCraftGrindstone_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftLoomActionData> {
    static bp::ItemStackRequestActionCraftLoom_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::CraftLoomActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData> {
    static bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::CraftResultsActionData> {
    static bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001> downgrade(
        bp::ItemStackRequestCereal_<2168>::CraftResultsActionData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestCereal_<2168>::RequestData> {
    static bp::ItemStackRequestData_<1001> downgrade(bp::ItemStackRequestCereal_<2168>::RequestData &&from);
};

template <>
struct Transformer<bp::ItemStackRequestPacket_<2168>> {
    static bp::ItemStackRequestPacket_<1001> downgrade(bp::ItemStackRequestPacket_<2168> &&from);
};

} // namespace endweave
