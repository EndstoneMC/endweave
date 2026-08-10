#include "endweave/protocols/v2168/crafting.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace ew = endweave;

namespace endweave {
namespace {

std::size_t gridCells(std::int32_t width, std::int32_t height)
{
    if (width <= 0 || height <= 0) {
        return 0;
    }
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

template <class Parent>
bp::CraftingDataEntry_<1001> shapedEntry(const Context<Parent> &ctx, bp::CraftingDataEntryType entry_type,
                                         bp::ShapedRecipePayload_<2168> &&from)
{
    bp::CraftingDataEntry_<1001> entry;
    entry.entry_type = entry_type;
    entry.shaped_recipe = ew::transform(ctx, std::move(from));
    return entry;
}

template <class Parent>
bp::CraftingDataEntry_<1001> shapelessEntry(const Context<Parent> &ctx, bp::CraftingDataEntryType entry_type,
                                            bp::ShapelessRecipePayload_<2168> &&from)
{
    bp::CraftingDataEntry_<1001> entry;
    entry.entry_type = entry_type;
    entry.shapeless_recipe = ew::transform(ctx, std::move(from));
    return entry;
}

} // namespace

void Transformer<bp::SerializedNetworkItemInstanceDescriptor_<2168>,
                 bp::SerializedNetworkItemInstanceDescriptor_<1001>>::
    transform(Context<bp::SerializedNetworkItemInstanceDescriptor_<1001>> &ctx,
              bp::SerializedNetworkItemInstanceDescriptor_<2168> &&from)
{
    auto &to = ctx.out();
    to.id = from.id;
    to.stack_size = from.stack_size;
    to.aux_value = from.aux_value;
    to.block_runtime_id = from.block_runtime_id;
    to.user_data_buffer = std::move(from.user_data_buffer);
}

void Transformer<bp::SerializedRecipeIngredient_<2168>, bp::SerializedRecipeIngredient_<1001>>::transform(
    Context<bp::SerializedRecipeIngredient_<1001>> &ctx, bp::SerializedRecipeIngredient_<2168> &&from)
{
    using InternalType = bp::ItemDescriptor_<1001>::InternalType;

    auto &to = ctx.out();
    // ENDWEAVE: the case reads back off the map's one key, keyed as BDS's own toMap writes it.
    // No key is the empty ingredient, which is what an invalid descriptor already means at 1001.
    if (auto item_tag = from.descriptor.find("item_tag"); item_tag != from.descriptor.end()) {
        to.descriptor.internal_type = InternalType::ITEM_TAG;
        to.descriptor.name = std::move(item_tag->second);
    }
    else if (auto molang = from.descriptor.find("tags"); molang != from.descriptor.end()) {
        // ENDWEAVE: "tags" is BDS's spelling of the molang key. The version comes back zero.
        to.descriptor.internal_type = InternalType::MOLANG;
        to.descriptor.name = std::move(molang->second);
    }
    else if (auto name = from.descriptor.find("name"); name != from.descriptor.end()) {
        // ENDWEAVE: BDS writes "name" for its id-coded default too, and 1001's deferred
        // descriptor is the one that carries a name and aux without an item registry.
        to.descriptor.internal_type = InternalType::DEFERRED;
        to.descriptor.name = std::move(name->second);
        to.descriptor.aux_value = from.aux_value;
    }
    to.stack_size = from.stack_size;
}

void Transformer<bp::SerializedRecipeUnlockingRequirement_<2168>, bp::SerializedRecipeUnlockingRequirement_<1001>>::
    transform(Context<bp::SerializedRecipeUnlockingRequirement_<1001>> &ctx,
              bp::SerializedRecipeUnlockingRequirement_<2168> &&from)
{
    auto &to = ctx.out();
    to.context = from.context;
    // ENDWEAVE: 1001 only reads the list under NONE, so an absent one is empty either way.
    to.ingredients = ew::transform_to<std::optional<std::vector<bp::SerializedRecipeIngredient_<1001>>>>(
                         ctx, std::move(from.ingredients))
                         .value_or(std::vector<bp::SerializedRecipeIngredient_<1001>>{});
}

void Transformer<bp::ShapedRecipePayload_<2168>, bp::ShapedRecipePayload_<1001>>::transform(
    Context<bp::ShapedRecipePayload_<1001>> &ctx, bp::ShapedRecipePayload_<2168> &&from)
{
    auto &to = ctx.out();
    to.recipe_id = std::move(from.recipe_id);
    to.width = from.width;
    to.height = from.height;
    to.ingredients = ew::transform(ctx, std::move(from.ingredients));
    // ENDWEAVE: 1001 writes no count and reads back exactly width * height, so any other
    // length desynchronises the packet. Short pads with the empty cell, long truncates.
    to.ingredients.resize(gridCells(to.width, to.height));
    to.results = ew::transform(ctx, std::move(from.results));
    to.uuid = from.uuid;
    to.tag = std::move(from.tag);
    to.priority = from.priority;
    to.assume_symmetry = from.assume_symmetry;
    // ENDWEAVE: default-constructed is the NONE context with no ingredients, which is what
    // BDS itself reads for the absent optional.
    to.unlocking_requirement = ew::transform_to<std::optional<bp::SerializedRecipeUnlockingRequirement_<1001>>>(
                                   ctx, std::move(from.unlocking_requirement))
                                   .value_or(bp::SerializedRecipeUnlockingRequirement_<1001>{});
    to.net_id = from.net_id;
}

void Transformer<bp::ShapelessRecipePayload_<2168>, bp::ShapelessRecipePayload_<1001>>::transform(
    Context<bp::ShapelessRecipePayload_<1001>> &ctx, bp::ShapelessRecipePayload_<2168> &&from)
{
    auto &to = ctx.out();
    to.recipe_id = std::move(from.recipe_id);
    to.ingredients = ew::transform(ctx, std::move(from.ingredients));
    to.results = ew::transform(ctx, std::move(from.results));
    to.uuid = from.uuid;
    to.tag = std::move(from.tag);
    to.priority = from.priority;
    // ENDWEAVE: default-constructed is the NONE context with no ingredients, which is what
    // BDS itself reads for the absent optional.
    to.unlocking_requirement = ew::transform_to<std::optional<bp::SerializedRecipeUnlockingRequirement_<1001>>>(
                                   ctx, std::move(from.unlocking_requirement))
                                   .value_or(bp::SerializedRecipeUnlockingRequirement_<1001>{});
    to.net_id = from.net_id;
}

void Transformer<bp::SmithingTransformRecipePayload_<2168>, bp::SmithingTransformRecipePayload_<1001>>::transform(
    Context<bp::SmithingTransformRecipePayload_<1001>> &ctx, bp::SmithingTransformRecipePayload_<2168> &&from)
{
    auto &to = ctx.out();
    to.recipe_id = std::move(from.recipe_id);
    to.template_ingredient = ew::transform(ctx, std::move(from.template_ingredient));
    to.base_ingredient = ew::transform(ctx, std::move(from.base_ingredient));
    to.addition_ingredient = ew::transform(ctx, std::move(from.addition_ingredient));
    to.result = ew::transform(ctx, std::move(from.result));
    to.tag = std::move(from.tag);
    to.net_id = from.net_id;
}

void Transformer<bp::SmithingTrimRecipePayload_<2168>, bp::SmithingTrimRecipePayload_<1001>>::transform(
    Context<bp::SmithingTrimRecipePayload_<1001>> &ctx, bp::SmithingTrimRecipePayload_<2168> &&from)
{
    auto &to = ctx.out();
    to.recipe_id = std::move(from.recipe_id);
    to.template_ingredient = ew::transform(ctx, std::move(from.template_ingredient));
    to.base_ingredient = ew::transform(ctx, std::move(from.base_ingredient));
    to.addition_ingredient = ew::transform(ctx, std::move(from.addition_ingredient));
    to.tag = std::move(from.tag);
    to.net_id = from.net_id;
}

void Transformer<bp::CraftingDataPacket_<2168>, bp::CraftingDataPacket_<1001>>::transform(
    Context<bp::CraftingDataPacket_<1001>> &ctx, bp::CraftingDataPacket_<2168> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: BDS's emission order is unrecoverable, so the eleven lists interleave in the
    // order the 2168 packet declares them, the only sequence the source carries.
    for (auto &recipe : from.shaped_recipes) {
        to.crafting_entries.push_back(shapedEntry(ctx, bp::CraftingDataEntryType::SHAPED_RECIPE, std::move(recipe)));
    }
    for (auto &recipe : from.shapeless_recipes) {
        to.crafting_entries.push_back(
            shapelessEntry(ctx, bp::CraftingDataEntryType::SHAPELESS_RECIPE, std::move(recipe)));
    }
    for (auto &recipe : from.multi_recipes) {
        bp::CraftingDataEntry_<1001> entry;
        entry.entry_type = bp::CraftingDataEntryType::MULTI_RECIPE;
        entry.multi_recipe = recipe;
        to.crafting_entries.push_back(entry);
    }
    for (auto &recipe : from.user_data_shapeless_recipes) {
        to.crafting_entries.push_back(
            shapelessEntry(ctx, bp::CraftingDataEntryType::USER_DATA_SHAPELESS_RECIPE, std::move(recipe)));
    }
    for (auto &recipe : from.shapeless_chemistry_recipes) {
        to.crafting_entries.push_back(
            shapelessEntry(ctx, bp::CraftingDataEntryType::SHAPELESS_CHEMISTRY_RECIPE, std::move(recipe)));
    }
    for (auto &recipe : from.shaped_chemistry_recipes) {
        to.crafting_entries.push_back(
            shapedEntry(ctx, bp::CraftingDataEntryType::SHAPED_CHEMISTRY_RECIPE, std::move(recipe)));
    }
    for (auto &recipe : from.smithing_transform_recipes) {
        bp::CraftingDataEntry_<1001> entry;
        entry.entry_type = bp::CraftingDataEntryType::SMITHING_TRANSFORM_RECIPE;
        entry.smithing_transform_recipe = ew::transform(ctx, std::move(recipe));
        to.crafting_entries.push_back(std::move(entry));
    }
    for (auto &recipe : from.smithing_trim_recipes) {
        bp::CraftingDataEntry_<1001> entry;
        entry.entry_type = bp::CraftingDataEntryType::SMITHING_TRIM_RECIPE;
        entry.smithing_trim_recipe = ew::transform(ctx, std::move(recipe));
        to.crafting_entries.push_back(std::move(entry));
    }
    to.potion_mix_entries = std::move(from.potion_mix_entries);
    to.container_mix_entries = std::move(from.container_mix_entries);
    to.material_reducer_entries = std::move(from.material_reducer_entries);
    to.clear_recipes = from.clear_recipes;
}

void Transformer<bp::CreativeGroupInfoPayload_<2168>, bp::CreativeGroupInfoPayload_<1001>>::transform(
    Context<bp::CreativeGroupInfoPayload_<1001>> &ctx, bp::CreativeGroupInfoPayload_<2168> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: one enum in both eras. 1001 writes the wider field, so nothing narrows.
    to.creative_item_category = from.creative_item_category;
    to.name = std::move(from.name);
    to.icon = ew::transform(ctx, std::move(from.icon));
}

void Transformer<bp::CreativeItemEntryPayload_<2168>, bp::CreativeItemEntryPayload_<1001>>::transform(
    Context<bp::CreativeItemEntryPayload_<1001>> &ctx, bp::CreativeItemEntryPayload_<2168> &&from)
{
    auto &to = ctx.out();
    to.creative_item_net_id = from.creative_item_net_id;
    to.item_descriptor = ew::transform(ctx, std::move(from.item_descriptor));
    to.group_index = from.group_index;
}

void Transformer<bp::CreativeContentPacket_<2168>, bp::CreativeContentPacket_<1001>>::transform(
    Context<bp::CreativeContentPacket_<1001>> &ctx, bp::CreativeContentPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.groups = ew::transform(ctx, std::move(from.groups));
    to.entries = ew::transform(ctx, std::move(from.entries));
}

} // namespace endweave
