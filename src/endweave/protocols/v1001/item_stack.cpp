#include "endweave/protocols/v1001/item_stack.h"

#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ew = endweave;

namespace {

using Cereal = bp::ItemStackRequestCereal_<2168>;
using ItemDescriptorData = decltype(Cereal::RecipeIngredientData::item_descriptor);

ItemDescriptorData upgradeItemDescriptor(bp::ItemDescriptor_<1001> &&from)
{
    using InternalType = bp::ItemDescriptor_<1001>::InternalType;
    // ENDWEAVE: 2168's four descriptor forms are all name-based; deferred is already a name plus
    // an aux value, so it is the one that survives exactly.
    switch (from.internal_type) {
    case InternalType::MOLANG:
        return Cereal::MolangItemDescriptorData{Cereal::ItemDescriptorType::MOLANG, std::move(from.name),
                                                static_cast<bp::MolangVersion>(from.molang_version)};
    case InternalType::ITEM_TAG:
        return Cereal::ItemTagDescriptorData{Cereal::ItemDescriptorType::ITEM_TAG, std::move(from.name)};
    case InternalType::DEFERRED:
    case InternalType::COMPLEX_ALIAS:
        return Cereal::ItemNameDescriptorData{Cereal::ItemDescriptorType::ITEM_NAME, std::move(from.name),
                                              from.aux_value};
    // ENDWEAVE: TODO a default descriptor numbers the item and 2168 has nowhere to put a number;
    // resolving it needs the server's item registry on the connection.
    case InternalType::DEFAULT:
        return Cereal::ItemNameDescriptorData{Cereal::ItemDescriptorType::ITEM_NAME, {}, from.aux_value};
    default:
        return Cereal::EmptyItemDescriptorData{Cereal::ItemDescriptorType::EMPTY};
    }
}

Cereal::RecipeIngredientData upgradeRecipeIngredient(bp::SerializedRecipeIngredient_<1001> &&from)
{
    Cereal::RecipeIngredientData to;
    to.item_descriptor = upgradeItemDescriptor(std::move(from.descriptor));
    to.stack_size = from.stack_size;
    return to;
}

Cereal::NetworkItemInstanceDescriptorData upgradeItemInstance(bp::SerializedNetworkItemInstanceDescriptor_<1001> &&from)
{
    Cereal::NetworkItemInstanceDescriptorData to;
    // ENDWEAVE: TODO as with a default descriptor -- id zero is BDS's empty item, anything else
    // keeps only its aux value until the connection carries an item registry.
    if (from.id == 0) {
        to.item_descriptor = Cereal::EmptyItemDescriptorData{Cereal::ItemDescriptorType::EMPTY};
    }
    else {
        to.item_descriptor = Cereal::ItemNameDescriptorData{
            Cereal::ItemDescriptorType::ITEM_NAME, {}, static_cast<std::int32_t>(from.aux_value)};
    }
    to.stack_size = from.stack_size;
    to.block_runtime_id = static_cast<std::uint32_t>(from.block_runtime_id);
    to.user_data_buffer = std::move(from.user_data_buffer);
    return to;
}

} // namespace

namespace endweave {

bp::RedactableString_<2168> Transformer<bp::RedactableString_<1001>, bp::RedactableString_<2168>>::transform(
    bp::RedactableString_<1001> &&from)
{
    bp::RedactableString_<2168> to;
    to.unredacted = std::move(from.unredacted);
    // ENDWEAVE: an empty redaction goes up absent, not present-and-empty -- same one zero byte.
    if (!from.redacted.empty()) {
        to.redacted = std::move(from.redacted);
    }
    return to;
}

bp::ItemStackResponseSlotInfo_<2168> Transformer<
    bp::ItemStackResponseSlotInfo_<1001>,
    bp::ItemStackResponseSlotInfo_<2168>>::transform(bp::ItemStackResponseSlotInfo_<1001> &&from)
{
    bp::ItemStackResponseSlotInfo_<2168> to;
    to.requested_slot = from.requested_slot;
    to.slot = from.slot;
    to.amount = from.amount;
    // ENDWEAVE: 1001 always carries a net id, so it is always present at 2168, zero included.
    to.item_stack_net_id = from.item_stack_net_id;
    to.custom_name = ew::transform(std::move(from.custom_name));
    to.durability_correction = from.durability_correction;
    return to;
}

bp::ItemStackResponseContainerInfo_<2168> Transformer<
    bp::ItemStackResponseContainerInfo_<1001>,
    bp::ItemStackResponseContainerInfo_<2168>>::transform(bp::ItemStackResponseContainerInfo_<1001> &&from)
{
    bp::ItemStackResponseContainerInfo_<2168> to;
    to.full_container_name = std::move(from.full_container_name);
    to.slots = ew::transform(std::move(from.slots));
    return to;
}

bp::ItemStackResponseInfo_<2168> Transformer<bp::ItemStackResponseInfo_<1001>, bp::ItemStackResponseInfo_<2168>>::
    transform(bp::ItemStackResponseInfo_<1001> &&from)
{
    bp::ItemStackResponseInfo_<2168> to;
    to.result = from.result;
    to.client_request_id = from.client_request_id;
    // ENDWEAVE: 1001's result gate becomes 2168's presence flag, so a failed response goes up
    // absent rather than empty.
    if (from.result == bp::ItemStackNetResult::SUCCESS) {
        to.containers = ew::transform(std::move(from.containers));
    }
    return to;
}

bp::ItemStackResponsePacket_<2168> Transformer<bp::ItemStackResponsePacket_<1001>, bp::ItemStackResponsePacket_<2168>>::
    transform(bp::ItemStackResponsePacket_<1001> &&from)
{
    bp::ItemStackResponsePacket_<2168> to;
    to.responses = ew::transform(std::move(from.responses));
    return to;
}

bp::ItemStackRequestCereal_<2168>::SlotInfoData Transformer<
    bp::ItemStackRequestSlotInfo_<1001>,
    bp::ItemStackRequestCereal_<2168>::SlotInfoData>::transform(bp::ItemStackRequestSlotInfo_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::SlotInfoData to;
    to.full_container_name = std::move(from.full_container_name);
    to.slot = from.slot;
    to.net_id_variant = from.net_id_variant;
    return to;
}

bp::ItemStackRequestCereal_<2168>::TakeActionData Transformer<
    bp::ItemStackRequestActionTake_<1001>,
    bp::ItemStackRequestCereal_<2168>::TakeActionData>::transform(bp::ItemStackRequestActionTake_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::TakeActionData to;
    // ENDWEAVE: 2168 restates the action type in every payload, where 1001 carried it in the list
    // tag alone, so each action names its own here.
    to.action_type = bp::ItemStackRequestActionType::TAKE;
    to.amount = from.amount;
    to.source = ew::transform(std::move(from.src));
    to.destination = ew::transform(std::move(from.dst));
    return to;
}

bp::ItemStackRequestCereal_<2168>::PlaceActionData Transformer<
    bp::ItemStackRequestActionPlace_<1001>,
    bp::ItemStackRequestCereal_<2168>::PlaceActionData>::transform(bp::ItemStackRequestActionPlace_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::PlaceActionData to;
    to.action_type = bp::ItemStackRequestActionType::PLACE;
    to.amount = from.amount;
    to.source = ew::transform(std::move(from.src));
    to.destination = ew::transform(std::move(from.dst));
    return to;
}

bp::ItemStackRequestCereal_<2168>::SwapActionData Transformer<
    bp::ItemStackRequestActionSwap_<1001>,
    bp::ItemStackRequestCereal_<2168>::SwapActionData>::transform(bp::ItemStackRequestActionSwap_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::SwapActionData to;
    to.action_type = bp::ItemStackRequestActionType::SWAP;
    to.source = ew::transform(std::move(from.src));
    to.destination = ew::transform(std::move(from.dst));
    return to;
}

bp::ItemStackRequestCereal_<2168>::DropActionData Transformer<
    bp::ItemStackRequestActionDrop_<1001>,
    bp::ItemStackRequestCereal_<2168>::DropActionData>::transform(bp::ItemStackRequestActionDrop_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::DropActionData to;
    to.action_type = bp::ItemStackRequestActionType::DROP;
    to.amount = from.amount;
    to.source = ew::transform(std::move(from.src));
    to.randomly = from.randomly;
    return to;
}

bp::ItemStackRequestCereal_<2168>::DestroyActionData Transformer<
    bp::ItemStackRequestActionDestroy_<1001>,
    bp::ItemStackRequestCereal_<2168>::DestroyActionData>::transform(bp::ItemStackRequestActionDestroy_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::DestroyActionData to;
    to.action_type = bp::ItemStackRequestActionType::DESTROY;
    to.amount = from.amount;
    to.source = ew::transform(std::move(from.src));
    return to;
}

bp::ItemStackRequestCereal_<2168>::ConsumeActionData Transformer<
    bp::ItemStackRequestActionConsume_<1001>,
    bp::ItemStackRequestCereal_<2168>::ConsumeActionData>::transform(bp::ItemStackRequestActionConsume_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::ConsumeActionData to;
    to.action_type = bp::ItemStackRequestActionType::CONSUME;
    to.amount = from.amount;
    to.source = ew::transform(std::move(from.src));
    return to;
}

bp::ItemStackRequestCereal_<2168>::CreateActionData Transformer<
    bp::ItemStackRequestActionCreate_<1001>,
    bp::ItemStackRequestCereal_<2168>::CreateActionData>::transform(bp::ItemStackRequestActionCreate_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::CreateActionData to;
    to.action_type = bp::ItemStackRequestActionType::CREATE;
    to.results_index = from.results_index;
    return to;
}

bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData Transformer<
    bp::ItemStackRequestActionLabTableCombine_<1001>, bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData>::
    transform(bp::ItemStackRequestActionLabTableCombine_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData to;
    to.action_type = bp::ItemStackRequestActionType::SCREEN_LAB_TABLE_COMBINE;
    return to;
}

bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData Transformer<
    bp::ItemStackRequestActionBeaconPayment_<1001>, bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData>::
    transform(bp::ItemStackRequestActionBeaconPayment_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData to;
    to.action_type = bp::ItemStackRequestActionType::SCREEN_BEACON_PAYMENT;
    to.primary_effect_id = from.primary_effect_id;
    to.secondary_effect_id = from.secondary_effect_id;
    return to;
}

bp::ItemStackRequestCereal_<2168>::MineBlockActionData Transformer<
    bp::ItemStackRequestActionMineBlock_<1001>,
    bp::ItemStackRequestCereal_<2168>::MineBlockActionData>::transform(bp::ItemStackRequestActionMineBlock_<1001>
                                                                           &&from)
{
    bp::ItemStackRequestCereal_<2168>::MineBlockActionData to;
    to.action_type = bp::ItemStackRequestActionType::SCREEN_HUD_MINE_BLOCK;
    to.slot = from.slot;
    to.predicted_durability = from.predicted_durability;
    to.net_id_variant = from.net_id_variant;
    return to;
}

bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData Transformer<
    bp::ItemStackRequestActionCraftRecipe_<1001>,
    bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData>::transform(bp::ItemStackRequestActionCraftRecipe_<1001>
                                                                             &&from)
{
    bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData to;
    to.action_type = bp::ItemStackRequestActionType::CRAFT_RECIPE;
    to.recipe_net_id = from.recipe_net_id;
    to.num_crafts = from.num_crafts;
    return to;
}

bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData Transformer<
    bp::ItemStackRequestActionCraftRecipeAuto_<1001>, bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData>::
    transform(bp::ItemStackRequestActionCraftRecipeAuto_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData to;
    to.action_type = bp::ItemStackRequestActionType::CRAFT_RECIPE_AUTO;
    to.recipe_net_id = from.recipe_net_id;
    to.num_crafts = from.num_crafts;
    // ENDWEAVE: 2168 dropped num_ingredients, which only restated the list length.
    to.ingredients.reserve(from.ingredients.size());
    for (auto &ingredient : from.ingredients) {
        to.ingredients.push_back(upgradeRecipeIngredient(std::move(ingredient)));
    }
    return to;
}

bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData Transformer<
    bp::ItemStackRequestActionCraftCreative_<1001>, bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData>::
    transform(bp::ItemStackRequestActionCraftCreative_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData to;
    to.action_type = bp::ItemStackRequestActionType::CRAFT_CREATIVE;
    // ENDWEAVE: 2168 sends the creative net id bare, without the CreativeItemNetId wrapper.
    to.creative_item_net_id = from.creative_item_net_id.raw_id;
    to.num_crafts = from.num_crafts;
    return to;
}

bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData Transformer<
    bp::ItemStackRequestActionCraftRecipeOptional_<1001>,
    bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData>::
    transform(bp::ItemStackRequestActionCraftRecipeOptional_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData to;
    to.action_type = bp::ItemStackRequestActionType::CRAFT_RECIPE_OPTIONAL;
    to.recipe_net_id = from.recipe_net_id;
    to.filtered_string_index = from.filtered_string_index;
    return to;
}

bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData Transformer<
    bp::ItemStackRequestActionCraftGrindstone_<1001>,
    bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData>::
    transform(bp::ItemStackRequestActionCraftGrindstone_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData to;
    // ENDWEAVE: same action under two names -- 1001's grindstone recipe is 2168's
    // repair-and-disenchant, both at action type 16.
    to.action_type = bp::ItemStackRequestActionType::CRAFT_REPAIR_AND_DISENCHANT;
    // ENDWEAVE: 2168 reads the recipe net id signed where 1001 reads it unsigned, same bits.
    to.recipe_net_id = static_cast<std::int32_t>(from.recipe_net_id);
    to.num_crafts = from.num_crafts;
    to.repair_cost = from.repair_cost;
    return to;
}

bp::ItemStackRequestCereal_<2168>::CraftLoomActionData Transformer<
    bp::ItemStackRequestActionCraftLoom_<1001>,
    bp::ItemStackRequestCereal_<2168>::CraftLoomActionData>::transform(bp::ItemStackRequestActionCraftLoom_<1001>
                                                                           &&from)
{
    bp::ItemStackRequestCereal_<2168>::CraftLoomActionData to;
    to.action_type = bp::ItemStackRequestActionType::CRAFT_LOOM;
    to.pattern_name_id = std::move(from.pattern_name_id);
    to.num_crafts = from.num_crafts;
    return to;
}

bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData Transformer<
    bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001>,
    bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData>::
    transform(bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData to;
    to.action_type = bp::ItemStackRequestActionType::CRAFT_NON_IMPLEMENTED_DEPRECATEDASKTYLAING;
    return to;
}

bp::ItemStackRequestCereal_<2168>::CraftResultsActionData Transformer<
    bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001>,
    bp::ItemStackRequestCereal_<2168>::CraftResultsActionData>::
    transform(bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::CraftResultsActionData to;
    to.action_type = bp::ItemStackRequestActionType::CRAFT_RESULTS_DEPRECATEDASKTYLAING;
    to.craft_results.reserve(from.craft_results.size());
    for (auto &result : from.craft_results) {
        to.craft_results.push_back(upgradeItemInstance(std::move(result)));
    }
    to.num_crafts = from.num_crafts;
    return to;
}

bp::ItemStackRequestCereal_<2168>::RequestData Transformer<
    bp::ItemStackRequestData_<1001>,
    bp::ItemStackRequestCereal_<2168>::RequestData>::transform(bp::ItemStackRequestData_<1001> &&from)
{
    bp::ItemStackRequestCereal_<2168>::RequestData to;
    to.client_request_id = from.client_request_id;
    to.actions.reserve(from.actions.size());
    // ENDWEAVE: the tag is the variant index at both versions and they disagree above Create, so
    // each action is pushed by alternative type and the variant places it.
    for (auto &action : from.actions) {
        std::visit(
            [&to](auto &data) {
                // ENDWEAVE: the two deprecated slots have no 2168 class, so an action in one is
                // dropped -- they are payload-less placeholders at 1001 too.
                if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(data)>, std::monostate>) {
                    to.actions.push_back(ew::transform(std::move(data)));
                }
            },
            action);
    }
    to.strings_to_filter = std::move(from.strings_to_filter);
    to.strings_to_filter_origin = from.strings_to_filter_origin;
    return to;
}

bp::ItemStackRequestPacket_<2168> Transformer<bp::ItemStackRequestPacket_<1001>, bp::ItemStackRequestPacket_<2168>>::
    transform(bp::ItemStackRequestPacket_<1001> &&from)
{
    bp::ItemStackRequestPacket_<2168> to;
    to.requests = ew::transform(std::move(from.requests));
    return to;
}

} // namespace endweave
