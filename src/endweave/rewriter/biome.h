#pragma once

#include "endweave/protocol/mapping.h"
#include "endweave/protocol/rewrite.h"

#include <bedrock/protocol/biome.h>
#include <bedrock/protocol/network.h>
#include <cstddef>
#include <cstdint>
#include <map>
#include <ranges>

namespace bp = bedrock::protocol;

namespace endweave {

/** The definitions of the biomes the client lacks are left out. The string list is sent whole, so
 * the definitions left keep every index into it. */
template <int From, int To>
    requires(MappingData<From, To>::mapsBiomes())
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::BiomeDefinitionList)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::BiomeDefinitionList)> &packet)
    {
        const auto &strings = packet.string_list.strings;
        std::erase_if(packet.biome_data, [&](const auto &entry) {
            const std::size_t index = entry.first;
            return index < strings.size() && !MappingData<From, To>::getNewBiomeName(strings[index]);
        });
        for (auto &definition : std::views::values(packet.biome_data)) {
            definition.id = static_cast<std::uint16_t>(MappingData<From, To>::getNewBiomeId(definition.id));
        }
    }
};

} // namespace endweave
