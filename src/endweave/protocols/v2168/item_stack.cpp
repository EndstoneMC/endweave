#include "endweave/protocols/v2168/item_stack.h"

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ew = endweave;

namespace {

using Cereal = bp::ItemStackRequestCereal_<2168>;
using ItemDescriptorData = decltype(Cereal::RecipeIngredientData::item_descriptor);

bp::ItemDescriptor_<1001> downgradeItemDescriptor(ItemDescriptorData &&from)
{
    using InternalType = bp::ItemDescriptor_<1001>::InternalType;
    bp::ItemDescriptor_<1001> to;
    // ENDWEAVE: a 2168 item name becomes a deferred descriptor, 1001's only name-and-aux form and
    // the one BDS resolves by name rather than by id.
    if (auto *name = std::get_if<Cereal::ItemNameDescriptorData>(&from)) {
        to.internal_type = InternalType::DEFERRED;
        to.name = std::move(name->full_name);
        to.aux_value = static_cast<std::int16_t>(name->aux_value);
    }
    else if (auto *molang = std::get_if<Cereal::MolangItemDescriptorData>(&from)) {
        to.internal_type = InternalType::MOLANG;
        to.name = std::move(molang->tag_expression);
        to.molang_version = static_cast<std::uint8_t>(molang->molang_version);
    }
    else if (auto *tag = std::get_if<Cereal::ItemTagDescriptorData>(&from)) {
        to.internal_type = InternalType::ITEM_TAG;
        to.name = std::move(tag->item_tag);
    }
    else {
        to.internal_type = InternalType::INVALID;
    }
    return to;
}

bp::SerializedRecipeIngredient_<1001> downgradeRecipeIngredient(Cereal::RecipeIngredientData &&from)
{
    bp::SerializedRecipeIngredient_<1001> to;
    to.descriptor = downgradeItemDescriptor(std::move(from.item_descriptor));
    to.stack_size = from.stack_size;
    return to;
}

bp::SerializedNetworkItemInstanceDescriptor_<1001> downgradeItemInstance(
    Cereal::NetworkItemInstanceDescriptorData &&from)
{
    const auto *name = std::get_if<Cereal::ItemNameDescriptorData>(&from.item_descriptor);
    bp::SerializedNetworkItemInstanceDescriptor_<1001> to;
    // ENDWEAVE: TODO 1001 numbers the item and an instance descriptor has no room for the name; id
    // zero is BDS's empty item, so it arrives empty rather than wrong.
    to.id = 0;
    to.stack_size = from.stack_size;
    // ENDWEAVE: only the item-name form has an aux value to hand back.
    to.aux_value = name != nullptr ? static_cast<std::uint32_t>(name->aux_value) : 0;
    // ENDWEAVE: 1001 reads the block runtime id signed and 2168 unsigned, same bits.
    to.block_runtime_id = static_cast<std::int32_t>(from.block_runtime_id);
    to.user_data_buffer = std::move(from.user_data_buffer);
    return to;
}

} // namespace

namespace endweave {

bp::RedactableString_<1001> Transformer<bp::RedactableString_<2168>, bp::RedactableString_<1001>>::transform(
    bp::RedactableString_<2168> &&from)
{
    bp::RedactableString_<1001> to;
    to.unredacted_string = std::move(from.unredacted_string);
    // ENDWEAVE: an absent redaction comes down empty, not as a copy of the unredacted text --
    // absent means there is no redacted form.
    to.redacted_string = std::move(from.redacted_string).value_or(std::string{});
    return to;
}

bp::ItemStackResponseSlotInfo_<1001> Transformer<
    bp::ItemStackResponseSlotInfo_<2168>,
    bp::ItemStackResponseSlotInfo_<1001>>::transform(bp::ItemStackResponseSlotInfo_<2168> &&from)
{
    bp::ItemStackResponseSlotInfo_<1001> to;
    to.requested_slot = from.requested_slot;
    to.slot = from.slot;
    to.amount = from.amount;
    // ENDWEAVE: an absent net id becomes zero, which is BDS's own "no net id".
    to.item_stack_net_id = from.item_stack_net_id.value_or(bp::ItemStackNetId{});
    to.custom_name = ew::transform(std::move(from.custom_name));
    to.durability_correction = from.durability_correction;
    return to;
}

bp::ItemStackResponseContainerInfo_<1001> Transformer<
    bp::ItemStackResponseContainerInfo_<2168>,
    bp::ItemStackResponseContainerInfo_<1001>>::transform(bp::ItemStackResponseContainerInfo_<2168> &&from)
{
    bp::ItemStackResponseContainerInfo_<1001> to;
    to.full_container_name = std::move(from.full_container_name);
    to.slots = ew::transform(std::move(from.slots));
    return to;
}

bp::ItemStackResponseInfo_<1001> Transformer<bp::ItemStackResponseInfo_<2168>, bp::ItemStackResponseInfo_<1001>>::
    transform(bp::ItemStackResponseInfo_<2168> &&from)
{
    bp::ItemStackResponseInfo_<1001> to;
    to.result = from.result;
    to.client_request_id = from.client_request_id;
    // ENDWEAVE: 1001 writes containers only on success, so a failing response that carries them
    // loses them -- there is no framing to put them in.
    to.containers = ew::transform_to<std::optional<std::vector<bp::ItemStackResponseContainerInfo_<1001>>>>(
                        std::move(from.containers))
                        .value_or(std::vector<bp::ItemStackResponseContainerInfo_<1001>>{});
    return to;
}

bp::ItemStackResponsePacket_<1001> Transformer<bp::ItemStackResponsePacket_<2168>, bp::ItemStackResponsePacket_<1001>>::
    transform(bp::ItemStackResponsePacket_<2168> &&from)
{
    bp::ItemStackResponsePacket_<1001> to;
    to.responses = ew::transform(std::move(from.responses));
    return to;
}

bp::ItemStackRequestSlotInfo_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::SlotInfoData,
    bp::ItemStackRequestSlotInfo_<1001>>::transform(bp::ItemStackRequestCereal_<2168>::SlotInfoData &&from)
{
    bp::ItemStackRequestSlotInfo_<1001> to;
    to.full_container_name = std::move(from.full_container_name);
    to.slot = from.slot;
    to.net_id_variant = from.net_id_variant;
    return to;
}

bp::ItemStackRequestActionTake_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::TakeActionData,
    bp::ItemStackRequestActionTake_<1001>>::transform(bp::ItemStackRequestCereal_<2168>::TakeActionData &&from)
{
    bp::ItemStackRequestActionTake_<1001> to;
    // ENDWEAVE: the restated action type is dropped; 1001 carries it in the list tag alone.
    to.amount = from.amount;
    to.src = ew::transform(std::move(from.source));
    to.dst = ew::transform(std::move(from.destination));
    return to;
}

bp::ItemStackRequestActionPlace_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::PlaceActionData,
    bp::ItemStackRequestActionPlace_<1001>>::transform(bp::ItemStackRequestCereal_<2168>::PlaceActionData &&from)
{
    bp::ItemStackRequestActionPlace_<1001> to;
    to.amount = from.amount;
    to.src = ew::transform(std::move(from.source));
    to.dst = ew::transform(std::move(from.destination));
    return to;
}

bp::ItemStackRequestActionSwap_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::SwapActionData,
    bp::ItemStackRequestActionSwap_<1001>>::transform(bp::ItemStackRequestCereal_<2168>::SwapActionData &&from)
{
    bp::ItemStackRequestActionSwap_<1001> to;
    to.src = ew::transform(std::move(from.source));
    to.dst = ew::transform(std::move(from.destination));
    return to;
}

bp::ItemStackRequestActionDrop_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::DropActionData,
    bp::ItemStackRequestActionDrop_<1001>>::transform(bp::ItemStackRequestCereal_<2168>::DropActionData &&from)
{
    bp::ItemStackRequestActionDrop_<1001> to;
    to.amount = from.amount;
    to.src = ew::transform(std::move(from.source));
    to.randomly = from.randomly;
    return to;
}

bp::ItemStackRequestActionDestroy_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::DestroyActionData,
    bp::ItemStackRequestActionDestroy_<1001>>::transform(bp::ItemStackRequestCereal_<2168>::DestroyActionData &&from)
{
    bp::ItemStackRequestActionDestroy_<1001> to;
    to.amount = from.amount;
    to.src = ew::transform(std::move(from.source));
    return to;
}

bp::ItemStackRequestActionConsume_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::ConsumeActionData,
    bp::ItemStackRequestActionConsume_<1001>>::transform(bp::ItemStackRequestCereal_<2168>::ConsumeActionData &&from)
{
    bp::ItemStackRequestActionConsume_<1001> to;
    to.amount = from.amount;
    to.src = ew::transform(std::move(from.source));
    return to;
}

bp::ItemStackRequestActionCreate_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::CreateActionData,
    bp::ItemStackRequestActionCreate_<1001>>::transform(bp::ItemStackRequestCereal_<2168>::CreateActionData &&from)
{
    bp::ItemStackRequestActionCreate_<1001> to;
    to.results_index = from.results_index;
    return to;
}

bp::ItemStackRequestActionLabTableCombine_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData, bp::ItemStackRequestActionLabTableCombine_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData &&from)
{
    return bp::ItemStackRequestActionLabTableCombine_<1001>{};
}

bp::ItemStackRequestActionBeaconPayment_<1001> Transformer<bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData,
                                                           bp::ItemStackRequestActionBeaconPayment_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData &&from)
{
    bp::ItemStackRequestActionBeaconPayment_<1001> to;
    to.primary_effect_id = from.primary_effect_id;
    to.secondary_effect_id = from.secondary_effect_id;
    return to;
}

bp::ItemStackRequestActionMineBlock_<1001> Transformer<bp::ItemStackRequestCereal_<2168>::MineBlockActionData,
                                                       bp::ItemStackRequestActionMineBlock_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::MineBlockActionData &&from)
{
    bp::ItemStackRequestActionMineBlock_<1001> to;
    to.slot = from.slot;
    to.predicted_durability = from.predicted_durability;
    to.net_id_variant = from.net_id_variant;
    return to;
}

bp::ItemStackRequestActionCraftRecipe_<1001> Transformer<bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData,
                                                         bp::ItemStackRequestActionCraftRecipe_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData &&from)
{
    bp::ItemStackRequestActionCraftRecipe_<1001> to;
    to.recipe_net_id = from.recipe_net_id;
    to.num_crafts = from.num_crafts;
    return to;
}

bp::ItemStackRequestActionCraftRecipeAuto_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData, bp::ItemStackRequestActionCraftRecipeAuto_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData &&from)
{
    bp::ItemStackRequestActionCraftRecipeAuto_<1001> to;
    to.recipe_net_id = from.recipe_net_id;
    to.num_crafts = from.num_crafts;
    // ENDWEAVE: 1001 wants a count 2168 stopped sending; it only restated the list length.
    to.num_ingredients = static_cast<std::uint8_t>(from.ingredients.size());
    to.ingredients.reserve(from.ingredients.size());
    for (auto &ingredient : from.ingredients) {
        to.ingredients.push_back(downgradeRecipeIngredient(std::move(ingredient)));
    }
    return to;
}

bp::ItemStackRequestActionCraftCreative_<1001> Transformer<bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData,
                                                           bp::ItemStackRequestActionCraftCreative_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData &&from)
{
    bp::ItemStackRequestActionCraftCreative_<1001> to;
    // ENDWEAVE: 1001 wraps the creative net id in CreativeItemNetId where 2168 sends it bare.
    to.creative_item_net_id = bp::CreativeItemNetId{from.creative_item_net_id};
    to.num_crafts = from.num_crafts;
    return to;
}

bp::ItemStackRequestActionCraftRecipeOptional_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData,
    bp::ItemStackRequestActionCraftRecipeOptional_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData &&from)
{
    bp::ItemStackRequestActionCraftRecipeOptional_<1001> to;
    to.recipe_net_id = from.recipe_net_id;
    to.filtered_string_index = from.filtered_string_index;
    return to;
}

bp::ItemStackRequestActionCraftGrindstone_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData,
    bp::ItemStackRequestActionCraftGrindstone_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData &&from)
{
    // ENDWEAVE: same action under two names -- 2168's repair-and-disenchant is 1001's grindstone
    // recipe, both at action type 16.
    bp::ItemStackRequestActionCraftGrindstone_<1001> to;
    // ENDWEAVE: 1001 reads the recipe net id unsigned where 2168 reads it signed, same bits.
    to.recipe_net_id = static_cast<std::uint32_t>(from.recipe_net_id);
    to.num_crafts = from.num_crafts;
    to.repair_cost = from.repair_cost;
    return to;
}

bp::ItemStackRequestActionCraftLoom_<1001> Transformer<bp::ItemStackRequestCereal_<2168>::CraftLoomActionData,
                                                       bp::ItemStackRequestActionCraftLoom_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::CraftLoomActionData &&from)
{
    bp::ItemStackRequestActionCraftLoom_<1001> to;
    to.pattern_name_id = std::move(from.pattern_name_id);
    to.num_crafts = from.num_crafts;
    return to;
}

bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData,
    bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData &&from)
{
    return bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001>{};
}

bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::CraftResultsActionData,
    bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001>>::
    transform(bp::ItemStackRequestCereal_<2168>::CraftResultsActionData &&from)
{
    bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001> to;
    to.craft_results.reserve(from.craft_results.size());
    for (auto &result : from.craft_results) {
        to.craft_results.push_back(downgradeItemInstance(std::move(result)));
    }
    to.num_crafts = from.num_crafts;
    return to;
}

bp::ItemStackRequestData_<1001> Transformer<
    bp::ItemStackRequestCereal_<2168>::RequestData,
    bp::ItemStackRequestData_<1001>>::transform(bp::ItemStackRequestCereal_<2168>::RequestData &&from)
{
    bp::ItemStackRequestData_<1001> to;
    to.client_request_id = from.client_request_id;
    to.actions.reserve(from.actions.size());
    // ENDWEAVE: the tag is the variant index at both versions and they disagree above Create, so
    // each action is pushed by alternative type and the variant places it.
    for (auto &action : from.actions) {
        std::visit(
            [&to](auto &data) {
                to.actions.push_back(ew::transform(std::move(data)));
            },
            action);
    }
    to.strings_to_filter = std::move(from.strings_to_filter);
    to.strings_to_filter_origin = from.strings_to_filter_origin;
    return to;
}

bp::ItemStackRequestPacket_<1001> Transformer<bp::ItemStackRequestPacket_<2168>, bp::ItemStackRequestPacket_<1001>>::
    transform(bp::ItemStackRequestPacket_<2168> &&from)
{
    bp::ItemStackRequestPacket_<1001> to;
    to.requests = ew::transform(std::move(from.requests));
    return to;
}

} // namespace endweave
