#pragma once

#include "endweave/protocol/mapping.h"
#include "endweave/protocol/rewrite.h"

#include <algorithm>
#include <bedrock/protocol/biome.h>
#include <bedrock/protocol/network.h>
#include <cstddef>
#include <cstdint>
#include <map>
#include <ranges>

namespace bp = bedrock::protocol;

namespace endweave {

/** The definitions of the biomes the client lacks are left out. The string list is sent whole, so
 * the definitions left keep every index into it. The surface blocks BDS types unsigned int, which
 * `rewriteBlockIds` cannot find, are rewritten by name. */
template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::BiomeDefinitionList)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::BiomeDefinitionList)> &packet)
    {
        if constexpr (MappingData<From, To>::mapsBiomes()) {
            const auto &strings = packet.string_list.strings;
            std::erase_if(packet.biome_data, [&](const auto &entry) {
                const std::size_t index = entry.first;
                return index < strings.size() && !MappingData<From, To>::getNewBiomeName(strings[index]);
            });
        }
        for (auto &definition : std::views::values(packet.biome_data)) {
            definition.id = static_cast<std::uint16_t>(MappingData<From, To>::getNewBiomeId(definition.id));
            if (!definition.chunk_gen_data) {
                continue;
            }
            const auto remap = [](std::uint32_t &id) {
                id = MappingData<From, To>::getNewBlockStateId(id);
            };
            auto &chunk_gen = *definition.chunk_gen_data;
            for (auto *surface : {&chunk_gen.surface_builder_data, &chunk_gen.subsurface_builder_data}) {
                if (!*surface) {
                    continue;
                }
                if (auto &capped = (*surface)->capped_surface) {
                    std::ranges::for_each(capped->floor_blocks, remap);
                    std::ranges::for_each(capped->ceiling_blocks, remap);
                    for (auto *block : {&capped->sea_block, &capped->foundation_block, &capped->beach_block}) {
                        if (*block) {
                            remap(**block);
                        }
                    }
                }
                if (auto &noise = (*surface)->noise_gradient_surface) {
                    std::ranges::for_each(noise->non_replaceable_blocks, remap);
                }
            }
        }
    }
};

} // namespace endweave
