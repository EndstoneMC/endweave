#pragma once

#include "endweave/protocol/rewrite.h"

#include <bedrock/protocol/dimension.h>
#include <bedrock/protocol/network.h>
#include <cstdint>
#include <map>
#include <optional>
#include <ranges>

namespace bp = bedrock::protocol;

namespace endweave {

/** How far a dimension reaches: `height` blocks up from `min_y`.
 * @see BDS DimensionDefinitionGroup::DimensionDefinition, ViaVersion DimensionData. */
struct DimensionData {
    std::int32_t min_y;
    std::int32_t height;
};

/** The dimensions a client knows, by dimension type: the overworld, the nether and the end as the
 * client has them built in, and each one a DimensionDataPacket defines.
 * @see ViaVersion EntityTracker#dimensionData. */
class DimensionDataStorage {
public:
    [[nodiscard]] std::optional<DimensionData> getDimensionData(std::int32_t dimension) const
    {
        const auto it = dimensions_.find(dimension);
        return it != dimensions_.end() ? std::optional{it->second} : std::nullopt;
    }

    void setDimensionData(std::int32_t dimension, DimensionData data)
    {
        dimensions_.insert_or_assign(dimension, data);
    }

private:
    // BDS VanillaDimensions: the overworld from y -64 to 320, the nether to 128 and the end to 256.
    std::map<std::int32_t, DimensionData> dimensions_ = {{0, {-64, 384}}, {1, {0, 128}}, {2, {0, 256}}};
};

template <int From, int To>
struct Rewriter<From, To, static_cast<int>(bp::MinecraftPacketIds::DimensionDataPacket)> {
    static void rewrite(bp::packet_of_t<From, static_cast<int>(bp::MinecraftPacketIds::DimensionDataPacket)> &packet,
                        DimensionDataStorage &dimensions)
    {
        for (const auto &definition : std::views::values(packet.dimension_definitions)) {
            if constexpr (requires { definition.min_y; }) {
                dimensions.setDimensionData(definition.dimension_type, {definition.min_y, definition.height_range});
            }
            else {
                dimensions.setDimensionData(
                    definition.dimension_type,
                    {definition.height_minimum, definition.height_maximum - definition.height_minimum});
            }
        }
    }
};

} // namespace endweave
