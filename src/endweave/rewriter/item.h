#pragma once

#include "endweave/protocol/mapping.h"
#include "endweave/protocol/rewrite.h"
#include "endweave/rewriter/block.h"

#include <algorithm>
#include <bedrock/protocol/crafting.h>
#include <bedrock/protocol/detail/reflect.hpp>
#include <bedrock/protocol/inventory.h>
#include <bedrock/protocol/item.h>
#include <bedrock/protocol/network.h>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace bp = bedrock::protocol;

namespace endweave {

/** The items of the server's ItemRegistryPacket that the client's version lacks, and the ones whose
 * aux value can name a potion it lacks, by the network id the server gave them, which creative
 * content and recipes name them by. */
class ItemRegistryStorage {
public:
    void setMissing(std::int32_t id)
    {
        missing_.insert(id);
    }

    [[nodiscard]] bool isMissing(std::int32_t id) const
    {
        return missing_.contains(id);
    }

    void setPotionItem(std::int32_t id, std::string name)
    {
        potion_items_.insert_or_assign(id, std::move(name));
    }

    [[nodiscard]] std::optional<std::string_view> getPotionItem(std::int32_t id) const
    {
        const auto it = potion_items_.find(id);
        return it != potion_items_.end() ? std::optional<std::string_view>{it->second} : std::nullopt;
    }

private:
    std::set<std::int32_t> missing_;
    std::map<std::int32_t, std::string> potion_items_;
};

namespace detail {

template <class T>
consteval bool isRecipeIngredient()
{
    if constexpr (bp::Reflected<T>) {
        return bp::struct_name<T>() == "SerializedRecipeIngredient" &&
               bp::field_index<T>("aux_value") != bp::field_npos;
    }
    else {
        return false;
    }
}

} // namespace detail

/** Keeps the items a client's version lacks out of what it is offered.
 * @see ViaVersion ItemRewriter. */
template <int From, int To>
struct ItemRewriter {
    /** Whether the item the server's registry numbers `id` is one `To` lacks, or at `aux` names a
     * potion `To` lacks. */
    static bool isMissing(const ItemRegistryStorage &storage, std::int32_t id, std::int32_t aux)
    {
        if (storage.isMissing(id)) {
            return true;
        }
        const auto name = storage.getPotionItem(id);
        return name && !MappingData<From, To>::getNewPotionAux(*name, aux);
    }

    /** Whether `value` names an item or potion `To` lacks, by the id the server's registry gave it or,
     * in a recipe ingredient, by name. */
    template <class T>
    static bool namesMissingItem(const T &value, const ItemRegistryStorage &storage)
    {
        if constexpr (detail::isItemInstanceDescriptor<T>()) {
            return isMissing(storage, value.id, static_cast<std::int32_t>(value.aux_value));
        }
        else if constexpr (detail::isRecipeIngredient<T>()) {
            const auto name = value.descriptor.find("name");
            return namesMissingItem(value.descriptor, storage) ||
                   (name != value.descriptor.end() &&
                    !MappingData<From, To>::getNewPotionAux(name->second, value.aux_value));
        }
        else if constexpr (std::is_same_v<T, std::map<std::string, std::string>>) {
            return std::ranges::any_of(value, [](const auto &entry) {
                return !MappingData<From, To>::getNewItemName(entry.second);
            });
        }
        else if constexpr (bp::Reflected<T>) {
            return [&]<std::size_t... I>(std::index_sequence<I...>) {
                return (namesMissingItem(bp::field_get<I>(value), storage) || ...);
            }(std::make_index_sequence<bp::field_count<T>()>{});
        }
        else if constexpr (requires { value.has_value(); }) {
            return value.has_value() && namesMissingItem(*value, storage);
        }
        else if constexpr (std::ranges::range<T> && !std::is_convertible_v<T, std::string_view>) {
            return std::ranges::any_of(value, [&](const auto &element) {
                return namesMissingItem(element, storage);
            });
        }
        else {
            return false;
        }
    }
};

/** The player UI has three more slots for the disc press at 2225. */
template <int From, int To>
    requires((From < 2225) != (To < 2225))
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::InventoryContent)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::InventoryContent)> &packet)
    {
        if (packet.inventory_id == bp::ContainerID::PlayerOnlyUi) {
            packet.slots.resize(To >= 2225 ? 57 : 54);
        }
    }
};

/** The client is still given every item the server registers, so an item it lacks that it comes
 * to hold resolves, but the ones it lacks, and the ones that carry a potion it may lack, are noted
 * for creative content and recipes. */
template <int From, int To>
    requires(MappingData<From, To>::mapsItems() || MappingData<From, To>::mapsPotions())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::ItemRegistryPacket)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::ItemRegistryPacket)> &packet,
                        ItemRegistryStorage &storage)
    {
        for (const auto &item : packet.items) {
            if (!MappingData<From, To>::getNewItemName(item.name)) {
                storage.setMissing(item.id);
            }
            if (MappingData<From, To>::mapsPotionsOf(item.name)) {
                storage.setPotionItem(item.id, item.name);
            }
        }
    }
};

/** The creative items the client lacks are left out, and so is a group they leave empty. */
template <int From, int To>
    requires(MappingData<From, To>::mapsItems() || MappingData<From, To>::mapsPotions())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::CreativeContent)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::CreativeContent)> &packet,
                        ItemRegistryStorage &storage)
    {
        const auto filled = [&] {
            std::vector<bool> filled(packet.groups.size());
            for (const auto &entry : packet.entries) {
                if (entry.group_index < filled.size()) {
                    filled[entry.group_index] = true;
                }
            }
            return filled;
        };
        const auto filled_before = filled();
        const auto missing = [&](const auto &descriptor) {
            return ItemRewriter<From, To>::isMissing(storage, descriptor.id,
                                                     static_cast<std::int32_t>(descriptor.aux_value));
        };
        std::erase_if(packet.entries, [&](const auto &entry) {
            return missing(entry.item_descriptor);
        });
        const auto filled_after = filled();

        std::vector<std::uint32_t> index(packet.groups.size());
        decltype(packet.groups) groups;
        for (std::uint32_t i = 0; i < packet.groups.size(); ++i) {
            if (filled_before[i] && !filled_after[i]) {
                continue;
            }
            index[i] = static_cast<std::uint32_t>(groups.size());
            auto &group = groups.emplace_back(std::move(packet.groups[i]));
            if (missing(group.icon)) {
                const auto first = std::ranges::find_if(packet.entries, [&](const auto &entry) {
                    return entry.group_index == i;
                });
                if (first != packet.entries.end()) {
                    group.icon = first->item_descriptor;
                }
            }
        }
        for (auto &entry : packet.entries) {
            if (entry.group_index < index.size()) {
                entry.group_index = index[entry.group_index];
            }
        }
        packet.groups = std::move(groups);
    }
};

/** The recipes and mixes that take or make an item or potion the client lacks are left out, and an
 * unlocking requirement that names one besides others loses it. */
template <int From, int To>
    requires(MappingData<From, To>::mapsItems() || MappingData<From, To>::mapsPotions())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::CraftingData)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::CraftingData)> &packet,
                        ItemRegistryStorage &storage)
    {
        const auto names = [&](const auto &recipe) {
            return ItemRewriter<From, To>::namesMissingItem(recipe, storage);
        };
        const auto unlock = [&](auto &recipes) {
            for (auto &recipe : recipes) {
                if (recipe.unlocking_requirement && recipe.unlocking_requirement->ingredients) {
                    auto &ingredients = *recipe.unlocking_requirement->ingredients;
                    if (!std::ranges::all_of(ingredients, names)) {
                        std::erase_if(ingredients, names);
                    }
                }
            }
        };
        unlock(packet.shaped_recipes);
        unlock(packet.shapeless_recipes);
        unlock(packet.user_data_shapeless_recipes);
        unlock(packet.shapeless_chemistry_recipes);
        unlock(packet.shaped_chemistry_recipes);
        std::erase_if(packet.shaped_recipes, names);
        std::erase_if(packet.shapeless_recipes, names);
        std::erase_if(packet.user_data_shapeless_recipes, names);
        std::erase_if(packet.shapeless_chemistry_recipes, names);
        std::erase_if(packet.shaped_chemistry_recipes, names);
        std::erase_if(packet.smithing_transform_recipes, names);
        std::erase_if(packet.smithing_trim_recipes, names);
        std::erase_if(packet.potion_mix_entries, [&](const auto &mix) {
            return ItemRewriter<From, To>::isMissing(storage, mix.from_item_id, mix.from_item_aux) ||
                   ItemRewriter<From, To>::isMissing(storage, mix.reagent_item_id, mix.reagent_item_aux) ||
                   ItemRewriter<From, To>::isMissing(storage, mix.to_item_id, mix.to_item_aux);
        });
        std::erase_if(packet.container_mix_entries, [&](const auto &mix) {
            return storage.isMissing(mix.from_item_id) || storage.isMissing(mix.reagent_item_id) ||
                   storage.isMissing(mix.to_item_id);
        });
        std::erase_if(packet.material_reducer_entries, [&](const auto &entry) {
            return storage.isMissing(entry.from_item_key >> 16) ||
                   std::ranges::any_of(entry.to_item_ids_and_counts, [&](const auto &output) {
                       return storage.isMissing(output.item_id);
                   });
        });
    }
};

} // namespace endweave
