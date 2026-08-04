#include "endweave/protocols/v1001/crafting.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

bp::SerializedNetworkItemInstanceDescriptor_<2168> Transformer<bp::SerializedNetworkItemInstanceDescriptor_<1001>>::
    upgrade(bp::SerializedNetworkItemInstanceDescriptor_<1001> &&from)
{
    bp::SerializedNetworkItemInstanceDescriptor_<2168> to;
    to.id = from.id;
    to.stack_size = from.stack_size;
    to.aux_value = from.aux_value;
    to.block_runtime_id = from.block_runtime_id;
    to.user_data_buffer = std::move(from.user_data_buffer);
    return to;
}

bp::SerializedRecipeIngredient_<2168> Transformer<bp::SerializedRecipeIngredient_<1001>>::upgrade(
    bp::SerializedRecipeIngredient_<1001> &&from)
{
    using InternalType = bp::ItemDescriptor_<1001>::InternalType;

    bp::SerializedRecipeIngredient_<2168> to;
    // ENDWEAVE: 2168 codes the descriptor as a one-key map, spelled as BDS's own toMap does.
    switch (from.descriptor.internal_type) {
    case InternalType::MOLANG:
        // ENDWEAVE: BDS spells this key "tags". One key leaves no room for the molang version.
        to.descriptor.emplace("tags", std::move(from.descriptor.name));
        break;
    case InternalType::ITEM_TAG:
        to.descriptor.emplace("item_tag", std::move(from.descriptor.name));
        break;
    case InternalType::DEFERRED:
    case InternalType::COMPLEX_ALIAS:
        // ENDWEAVE: BDS maps default, deferred and alias alike to "name", so they collapse.
        to.descriptor.emplace("name", std::move(from.descriptor.name));
        break;
    // ENDWEAVE: TODO the default case is id-coded at 1001 and name-coded at 2168. Resolving it
    // needs an item registry, so the ingredient crosses empty rather than naming a wrong item.
    case InternalType::DEFAULT:
    case InternalType::INVALID:
        break;
    }
    to.aux_value = from.descriptor.aux_value;
    to.stack_size = from.stack_size;
    return to;
}

bp::SerializedRecipeUnlockingRequirement_<2168> Transformer<bp::SerializedRecipeUnlockingRequirement_<1001>>::upgrade(
    bp::SerializedRecipeUnlockingRequirement_<1001> &&from)
{
    bp::SerializedRecipeUnlockingRequirement_<2168> to;
    to.context = from.context;
    // ENDWEAVE: 1001 only writes the list under NONE, so that is when 2168's optional is set.
    if (from.context == bp::SerializedRecipeUnlockingRequirement_<1001>::UnlockingContext::NONE) {
        to.ingredients = ew::upgrade(from.ingredients);
    }
    return to;
}

bp::ShapedRecipePayload_<2168> Transformer<bp::ShapedRecipePayload_<1001>>::upgrade(
    bp::ShapedRecipePayload_<1001> &&from)
{
    bp::ShapedRecipePayload_<2168> to;
    to.recipe_id = std::move(from.recipe_id);
    to.width = from.width;
    to.height = from.height;
    to.ingredients = ew::upgrade(from.ingredients);
    to.results = ew::upgrade(from.results);
    to.recipe_uuid = from.recipe_uuid;
    to.tag = std::move(from.tag);
    to.priority = from.priority;
    to.assume_symmetry = from.assume_symmetry;
    // ENDWEAVE: 1001 wrote the requirement unconditionally, so the optional is always set.
    to.unlocking_requirement = ew::upgrade(from.unlocking_requirement);
    to.net_id = from.net_id;
    return to;
}

bp::ShapelessRecipePayload_<2168> Transformer<bp::ShapelessRecipePayload_<1001>>::upgrade(
    bp::ShapelessRecipePayload_<1001> &&from)
{
    bp::ShapelessRecipePayload_<2168> to;
    to.recipe_id = std::move(from.recipe_id);
    to.ingredients = ew::upgrade(from.ingredients);
    to.results = ew::upgrade(from.results);
    to.recipe_uuid = from.recipe_uuid;
    to.tag = std::move(from.tag);
    to.priority = from.priority;
    // ENDWEAVE: 1001 wrote the requirement unconditionally, so the optional is always set.
    to.unlocking_requirement = ew::upgrade(from.unlocking_requirement);
    to.net_id = from.net_id;
    return to;
}

bp::SmithingTransformRecipePayload_<2168> Transformer<bp::SmithingTransformRecipePayload_<1001>>::upgrade(
    bp::SmithingTransformRecipePayload_<1001> &&from)
{
    bp::SmithingTransformRecipePayload_<2168> to;
    to.recipe_id = std::move(from.recipe_id);
    to.template_ingredient = ew::upgrade(from.template_ingredient);
    to.base_ingredient = ew::upgrade(from.base_ingredient);
    to.addition_ingredient = ew::upgrade(from.addition_ingredient);
    to.result = ew::upgrade(from.result);
    to.tag = std::move(from.tag);
    to.net_id = from.net_id;
    return to;
}

bp::SmithingTrimRecipePayload_<2168> Transformer<bp::SmithingTrimRecipePayload_<1001>>::upgrade(
    bp::SmithingTrimRecipePayload_<1001> &&from)
{
    bp::SmithingTrimRecipePayload_<2168> to;
    to.recipe_id = std::move(from.recipe_id);
    to.template_ingredient = ew::upgrade(from.template_ingredient);
    to.base_ingredient = ew::upgrade(from.base_ingredient);
    to.addition_ingredient = ew::upgrade(from.addition_ingredient);
    to.tag = std::move(from.tag);
    to.net_id = from.net_id;
    return to;
}

bp::CraftingDataPacket_<2168> Transformer<bp::CraftingDataPacket_<1001>>::upgrade(bp::CraftingDataPacket_<1001> &&from)
{
    bp::CraftingDataPacket_<2168> to;
    // ENDWEAVE: the entry tag is the only thing saying which of 2168's eleven lists a recipe
    // belongs in. Each list keeps the tagged sequence's relative order, so the pair round-trips.
    for (auto &entry : from.crafting_entries) {
        switch (entry.entry_type) {
        case bp::CraftingDataEntryType::SHAPELESS_RECIPE:
            to.shapeless_recipes.push_back(ew::upgrade(entry.shapeless_recipe));
            break;
        case bp::CraftingDataEntryType::SHAPED_RECIPE:
            to.shaped_recipes.push_back(ew::upgrade(entry.shaped_recipe));
            break;
        case bp::CraftingDataEntryType::MULTI_RECIPE:
            to.multi_recipes.push_back(entry.multi_recipe);
            break;
        case bp::CraftingDataEntryType::USER_DATA_SHAPELESS_RECIPE:
            to.user_data_shapeless_recipes.push_back(ew::upgrade(entry.shapeless_recipe));
            break;
        case bp::CraftingDataEntryType::SHAPELESS_CHEMISTRY_RECIPE:
            to.shapeless_chemistry_recipes.push_back(ew::upgrade(entry.shapeless_recipe));
            break;
        case bp::CraftingDataEntryType::SHAPED_CHEMISTRY_RECIPE:
            to.shaped_chemistry_recipes.push_back(ew::upgrade(entry.shaped_recipe));
            break;
        case bp::CraftingDataEntryType::SMITHING_TRANSFORM_RECIPE:
            to.smithing_transform_recipes.push_back(ew::upgrade(entry.smithing_transform_recipe));
            break;
        case bp::CraftingDataEntryType::SMITHING_TRIM_RECIPE:
            to.smithing_trim_recipes.push_back(ew::upgrade(entry.smithing_trim_recipe));
            break;
        // ENDWEAVE: COUNT terminates the enum rather than naming a recipe, so it carries nothing.
        case bp::CraftingDataEntryType::COUNT:
            break;
        }
    }
    to.potion_mixes = std::move(from.potion_mix_entries);
    to.container_mixes = std::move(from.container_mix_entries);
    to.material_reducers = std::move(from.material_reducer_entries);
    to.clear_recipes = from.clear_recipes;
    return to;
}

bp::CreativeGroupInfoPayload_<2168> Transformer<bp::CreativeGroupInfoPayload_<1001>>::upgrade(
    bp::CreativeGroupInfoPayload_<1001> &&from)
{
    bp::CreativeGroupInfoPayload_<2168> to;
    // ENDWEAVE: one enum in both eras. Only the wire width moved, so the copy loses nothing.
    to.creative_category = from.creative_category;
    to.name = std::move(from.name);
    to.group_icon_item = ew::upgrade(from.group_icon_item);
    return to;
}

bp::CreativeItemEntryPayload_<2168> Transformer<bp::CreativeItemEntryPayload_<1001>>::upgrade(
    bp::CreativeItemEntryPayload_<1001> &&from)
{
    bp::CreativeItemEntryPayload_<2168> to;
    to.creative_net_id = from.creative_net_id;
    to.item_instance = ew::upgrade(from.item_instance);
    to.group_index = from.group_index;
    return to;
}

bp::CreativeContentPacket_<2168> Transformer<bp::CreativeContentPacket_<1001>>::upgrade(
    bp::CreativeContentPacket_<1001> &&from)
{
    bp::CreativeContentPacket_<2168> to;
    to.groups = ew::upgrade(from.groups);
    to.entries = ew::upgrade(from.entries);
    return to;
}

} // namespace endweave
