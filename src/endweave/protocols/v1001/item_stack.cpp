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

void Transformer<bp::RedactableString_<1001>, bp::RedactableString_<2168>>::transform(
    Context<bp::RedactableString_<2168>> &ctx, bp::RedactableString_<1001> &&from)
{
    auto &to = ctx.out();
    to.unredacted_string = std::move(from.unredacted_string);
    // ENDWEAVE: an empty redaction goes up absent, not present-and-empty -- same one zero byte.
    if (!from.redacted_string.empty()) {
        to.redacted_string = std::move(from.redacted_string);
    }
}

void Transformer<bp::ItemStackResponseSlotInfo_<1001>, bp::ItemStackResponseSlotInfo_<2168>>::transform(
    Context<bp::ItemStackResponseSlotInfo_<2168>> &ctx, bp::ItemStackResponseSlotInfo_<1001> &&from)
{
    auto &to = ctx.out();
    to.requested_slot = from.requested_slot;
    to.slot = from.slot;
    to.amount = from.amount;
    // ENDWEAVE: 1001 always carries a net id, so it is always present at 2168, zero included.
    to.item_stack_net_id = from.item_stack_net_id;
    to.custom_name = ew::transform(ctx, std::move(from.custom_name));
    to.durability_correction = from.durability_correction;
}

void Transformer<bp::ItemStackResponseInfo_<1001>, bp::ItemStackResponseInfo_<2168>>::transform(
    Context<bp::ItemStackResponseInfo_<2168>> &ctx, bp::ItemStackResponseInfo_<1001> &&from)
{
    auto &to = ctx.out();
    to.result = from.result;
    to.client_request_id = from.client_request_id;
    // ENDWEAVE: 1001's result gate becomes 2168's presence flag, so a failed response goes up
    // absent rather than empty.
    if (from.result == bp::ItemStackNetResult::SUCCESS) {
        to.containers = ew::transform(ctx, std::move(from.containers));
    }
}

void Transformer<bp::ItemStackRequestSlotInfo_<1001>, bp::ItemStackRequestCereal_<2168>::SlotInfoData>::transform(
    Context<bp::ItemStackRequestCereal_<2168>::SlotInfoData> &ctx, bp::ItemStackRequestSlotInfo_<1001> &&from)
{
    auto &to = ctx.out();
    to.full_container_name = std::move(from.full_container_name);
    to.slot = from.slot;
    to.net_id_variant = from.net_id_variant;
}

void Transformer<bp::ItemStackRequestActionTake_<1001>, bp::ItemStackRequestCereal_<2168>::TakeActionData>::transform(
    Context<bp::ItemStackRequestCereal_<2168>::TakeActionData> &ctx, bp::ItemStackRequestActionTake_<1001> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: 2168 restates the action type in every payload, where 1001 carried it in the list
    // tag alone, so each action names its own here.
    to.action_type = bp::ItemStackRequestActionType::TAKE;
    to.amount = from.amount;
    to.source = ew::transform(ctx, std::move(from.src));
    to.destination = ew::transform(ctx, std::move(from.dst));
}

void Transformer<bp::ItemStackRequestActionPlace_<1001>, bp::ItemStackRequestCereal_<2168>::PlaceActionData>::transform(
    Context<bp::ItemStackRequestCereal_<2168>::PlaceActionData> &ctx, bp::ItemStackRequestActionPlace_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::PLACE;
    to.amount = from.amount;
    to.source = ew::transform(ctx, std::move(from.src));
    to.destination = ew::transform(ctx, std::move(from.dst));
}

void Transformer<bp::ItemStackRequestActionSwap_<1001>, bp::ItemStackRequestCereal_<2168>::SwapActionData>::transform(
    Context<bp::ItemStackRequestCereal_<2168>::SwapActionData> &ctx, bp::ItemStackRequestActionSwap_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::SWAP;
    to.source = ew::transform(ctx, std::move(from.src));
    to.destination = ew::transform(ctx, std::move(from.dst));
}

void Transformer<bp::ItemStackRequestActionDrop_<1001>, bp::ItemStackRequestCereal_<2168>::DropActionData>::transform(
    Context<bp::ItemStackRequestCereal_<2168>::DropActionData> &ctx, bp::ItemStackRequestActionDrop_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::DROP;
    to.amount = from.amount;
    to.source = ew::transform(ctx, std::move(from.src));
    to.randomly = from.randomly;
}

void Transformer<bp::ItemStackRequestActionDestroy_<1001>, bp::ItemStackRequestCereal_<2168>::DestroyActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::DestroyActionData> &ctx,
              bp::ItemStackRequestActionDestroy_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::DESTROY;
    to.amount = from.amount;
    to.source = ew::transform(ctx, std::move(from.src));
}

void Transformer<bp::ItemStackRequestActionConsume_<1001>, bp::ItemStackRequestCereal_<2168>::ConsumeActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::ConsumeActionData> &ctx,
              bp::ItemStackRequestActionConsume_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::CONSUME;
    to.amount = from.amount;
    to.source = ew::transform(ctx, std::move(from.src));
}

void Transformer<bp::ItemStackRequestActionCreate_<1001>, bp::ItemStackRequestCereal_<2168>::CreateActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::CreateActionData> &ctx,
              bp::ItemStackRequestActionCreate_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::CREATE;
    to.results_index = from.results_index;
}

void Transformer<bp::ItemStackRequestActionLabTableCombine_<1001>,
                 bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::LabTableCombineActionData> &ctx,
              bp::ItemStackRequestActionLabTableCombine_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::SCREEN_LAB_TABLE_COMBINE;
}

void Transformer<bp::ItemStackRequestActionBeaconPayment_<1001>,
                 bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::BeaconPaymentActionData> &ctx,
              bp::ItemStackRequestActionBeaconPayment_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::SCREEN_BEACON_PAYMENT;
    to.primary_effect_id = from.primary_effect_id;
    to.secondary_effect_id = from.secondary_effect_id;
}

void Transformer<bp::ItemStackRequestActionMineBlock_<1001>, bp::ItemStackRequestCereal_<2168>::MineBlockActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::MineBlockActionData> &ctx,
              bp::ItemStackRequestActionMineBlock_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::SCREEN_HUD_MINE_BLOCK;
    to.slot = from.slot;
    to.predicted_durability = from.predicted_durability;
    to.net_id_variant = from.net_id_variant;
}

void Transformer<bp::ItemStackRequestActionCraftRecipe_<1001>,
                 bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::CraftRecipeActionData> &ctx,
              bp::ItemStackRequestActionCraftRecipe_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::CRAFT_RECIPE;
    to.recipe_net_id = from.recipe_net_id;
    to.num_crafts = from.num_crafts;
}

void Transformer<bp::ItemStackRequestActionCraftRecipeAuto_<1001>,
                 bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::CraftRecipeAutoActionData> &ctx,
              bp::ItemStackRequestActionCraftRecipeAuto_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::CRAFT_RECIPE_AUTO;
    to.recipe_net_id = from.recipe_net_id;
    to.num_crafts = from.num_crafts;
    // ENDWEAVE: 2168 dropped num_ingredients, which only restated the list length.
    to.ingredients.reserve(from.ingredients.size());
    for (auto &ingredient : from.ingredients) {
        to.ingredients.push_back(upgradeRecipeIngredient(std::move(ingredient)));
    }
}

void Transformer<bp::ItemStackRequestActionCraftCreative_<1001>,
                 bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::CraftCreativeActionData> &ctx,
              bp::ItemStackRequestActionCraftCreative_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::CRAFT_CREATIVE;
    // ENDWEAVE: 2168 sends the creative net id bare, without the CreativeItemNetId wrapper.
    to.creative_item_net_id = from.creative_item_net_id.raw_id;
    to.num_crafts = from.num_crafts;
}

void Transformer<bp::ItemStackRequestActionCraftRecipeOptional_<1001>,
                 bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::CraftRecipeOptionalActionData> &ctx,
              bp::ItemStackRequestActionCraftRecipeOptional_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::CRAFT_RECIPE_OPTIONAL;
    to.recipe_net_id = from.recipe_net_id;
    to.filtered_string_index = from.filtered_string_index;
}

void Transformer<bp::ItemStackRequestActionCraftGrindstone_<1001>,
                 bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::CraftRepairAndDisenchantActionData> &ctx,
              bp::ItemStackRequestActionCraftGrindstone_<1001> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: same action under two names -- 1001's grindstone recipe is 2168's
    // repair-and-disenchant, both at action type 16.
    to.action_type = bp::ItemStackRequestActionType::CRAFT_REPAIR_AND_DISENCHANT;
    // ENDWEAVE: 2168 reads the recipe net id signed where 1001 reads it unsigned, same bits.
    to.recipe_net_id = static_cast<std::int32_t>(from.recipe_net_id);
    to.num_crafts = from.num_crafts;
    to.repair_cost = from.repair_cost;
}

void Transformer<bp::ItemStackRequestActionCraftLoom_<1001>, bp::ItemStackRequestCereal_<2168>::CraftLoomActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::CraftLoomActionData> &ctx,
              bp::ItemStackRequestActionCraftLoom_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::CRAFT_LOOM;
    to.pattern_name_id = std::move(from.pattern_name_id);
    to.num_crafts = from.num_crafts;
}

void Transformer<bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001>,
                 bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::CraftNonImplementedActionData> &ctx,
              bp::ItemStackRequestActionCraftNonImplemented_DEPRECATEDASKTYLAING_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::CRAFT_NON_IMPLEMENTED_DEPRECATEDASKTYLAING;
}

void Transformer<bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001>,
                 bp::ItemStackRequestCereal_<2168>::CraftResultsActionData>::
    transform(Context<bp::ItemStackRequestCereal_<2168>::CraftResultsActionData> &ctx,
              bp::ItemStackRequestActionCraftResults_DEPRECATEDASKTYLAING_<1001> &&from)
{
    auto &to = ctx.out();
    to.action_type = bp::ItemStackRequestActionType::CRAFT_RESULTS_DEPRECATEDASKTYLAING;
    to.craft_results.reserve(from.craft_results.size());
    for (auto &result : from.craft_results) {
        to.craft_results.push_back(upgradeItemInstance(std::move(result)));
    }
    to.num_crafts = from.num_crafts;
}

void Transformer<bp::ItemStackRequestData_<1001>, bp::ItemStackRequestCereal_<2168>::RequestData>::transform(
    Context<bp::ItemStackRequestCereal_<2168>::RequestData> &ctx, bp::ItemStackRequestData_<1001> &&from)
{
    auto &to = ctx.out();
    to.client_request_id = from.client_request_id;
    to.actions.reserve(from.actions.size());
    // ENDWEAVE: the tag is the variant index at both versions and they disagree above Create, so
    // each action is pushed by alternative type and the variant places it.
    for (auto &action : from.actions) {
        std::visit(
            [&to, &ctx](auto &data) {
                // ENDWEAVE: the two deprecated slots have no 2168 class, so an action in one is
                // dropped -- they are payload-less placeholders at 1001 too.
                if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(data)>, std::monostate>) {
                    to.actions.push_back(ew::transform(ctx, std::move(data)));
                }
            },
            action);
    }
    to.strings_to_filter = std::move(from.strings_to_filter);
    to.strings_to_filter_origin = from.strings_to_filter_origin;
}

} // namespace endweave
