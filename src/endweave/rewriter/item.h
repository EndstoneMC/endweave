#pragma once

#include "endweave/protocol/mapping.h"
#include "endweave/protocol/rewrite.h"
#include "endweave/rewriter/block.h"

#include <algorithm>
#include <bedrock/protocol/crafting.h>
#include <bedrock/protocol/detail/reflect.hpp>
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

/** The items of the server's ItemRegistryPacket that the client's version lacks, by the network id
 * the server gave them, which creative content and recipes name them by. */
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

private:
    std::set<std::int32_t> missing_;
};

/** Keeps the items a client's version lacks out of what it is offered.
 * @see ViaVersion ItemRewriter. */
template <int From, int To>
struct ItemRewriter {
    /** Whether `value` names an item `To` lacks, by the id the server's registry gave it or, in a
     * recipe ingredient, by name. */
    template <class T>
    static bool namesMissingItem(const T &value, const ItemRegistryStorage &storage)
    {
        if constexpr (detail::isItemInstanceDescriptor<T>()) {
            return storage.isMissing(value.id);
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

/** The client is still given every item the server registers, so an item it lacks that it comes
 * to hold resolves, but the ones it lacks are noted for creative content and recipes. */
template <int From, int To>
    requires(MappingData<From, To>::mapsItems())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::ItemRegistryPacket)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::ItemRegistryPacket)> &packet,
                        ItemRegistryStorage &storage)
    {
        for (const auto &item : packet.items) {
            if (!MappingData<From, To>::getNewItemName(item.name)) {
                storage.setMissing(item.id);
            }
        }
    }
};

/** The creative items the client lacks are left out, and so is a group they leave empty. */
template <int From, int To>
    requires(MappingData<From, To>::mapsItems())
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
        std::erase_if(packet.entries, [&](const auto &entry) {
            return storage.isMissing(entry.item_descriptor.id);
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
            if (storage.isMissing(group.icon.id)) {
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

/** The recipes and mixes that take or make an item the client lacks are left out. */
template <int From, int To>
    requires(MappingData<From, To>::mapsItems())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::CraftingData)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::CraftingData)> &packet,
                        ItemRegistryStorage &storage)
    {
        const auto names = [&](const auto &recipe) {
            return ItemRewriter<From, To>::namesMissingItem(recipe, storage);
        };
        std::erase_if(packet.shaped_recipes, names);
        std::erase_if(packet.shapeless_recipes, names);
        std::erase_if(packet.user_data_shapeless_recipes, names);
        std::erase_if(packet.shapeless_chemistry_recipes, names);
        std::erase_if(packet.shaped_chemistry_recipes, names);
        std::erase_if(packet.smithing_transform_recipes, names);
        std::erase_if(packet.smithing_trim_recipes, names);
        std::erase_if(packet.potion_mix_entries, [&](const auto &mix) {
            return storage.isMissing(mix.from_item_id) || storage.isMissing(mix.reagent_item_id) ||
                   storage.isMissing(mix.to_item_id);
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
