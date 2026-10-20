#pragma once

#include "block_connections.h"
#include "endweave/protocol/block_connection.h"
#include "mappings.h"

#include <algorithm>
#include <array>
#include <bedrock/protocol/common.h>
#include <bedrock/protocol/nbt.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace endweave {

/** minecraft:corner, the shape a stair takes from the stairs in front of and behind it.
 * @see BDS CornerBehaviorAnon::CornerConnectionCallback, ViaVersion StairConnectionHandler. */
class StairConnectionHandler {
public:
    template <class Add>
    static void init(Add &&add)
    {
        for (const auto &[id, stair] : getStairs()) {
            add(id);
        }
    }

    template <class UserBlockData>
    static std::uint32_t connect(const bp::BlockPos &pos, std::uint32_t id, UserBlockData &&blocks)
    {
        const auto *self = find(id);
        if (self == nullptr) {
            return id;
        }
        const auto stair_at = [&](const bp::BlockPos &offset) -> const Stair * {
            const auto block = blocks(bp::BlockPos{pos.x + offset.x, pos.y + offset.y, pos.z + offset.z});
            return block ? find(*block) : nullptr;
        };
        const auto same = [&](const bp::BlockPos &offset) {
            const auto *stair = stair_at(offset);
            return stair != nullptr && stair->base == self->base;
        };

        const auto &config = DIRECTION_CONFIGS[static_cast<std::size_t>(self->direction)];
        if (const auto *back = stair_at(config.back); back != nullptr && back->upside_down == self->upside_down) {
            if (back->direction == config.left.direction && !same(config.left.outer_lock)) {
                return self->shapes[static_cast<std::size_t>(CornerShape::OuterLeft)];
            }
            if (back->direction == config.right.direction && !same(config.right.outer_lock)) {
                return self->shapes[static_cast<std::size_t>(CornerShape::OuterRight)];
            }
        }
        if (const auto *front = stair_at(config.front); front != nullptr && front->upside_down == self->upside_down) {
            if (front->direction == config.left.direction && !same(config.left.inner_lock)) {
                return self->shapes[static_cast<std::size_t>(CornerShape::InnerLeft)];
            }
            if (front->direction == config.right.direction && !same(config.right.inner_lock)) {
                return self->shapes[static_cast<std::size_t>(CornerShape::InnerRight)];
            }
        }
        return self->shapes[static_cast<std::size_t>(CornerShape::None)];
    }

private:
    // BDS Direction::Type.
    enum class Direction : std::uint8_t {
        South,
        West,
        North,
        East,
    };

    // BDS CornerShape, as StateSerializationUtils::CORNER_SHAPE_MAPPER names it.
    enum class CornerShape : std::uint8_t {
        None,
        InnerLeft,
        InnerRight,
        OuterLeft,
        OuterRight,
    };
    static constexpr std::array<std::string_view, 5> CORNER_SHAPE_NAMES = {"none", "inner_left", "inner_right",
                                                                           "outer_left", "outer_right"};

    struct Side {
        Direction direction;
        bp::BlockPos outer_lock;
        bp::BlockPos inner_lock;
    };

    struct DirectionConfig {
        bp::BlockPos back;
        bp::BlockPos front;
        Side left;
        Side right;
    };

    // BDS CornerBehaviorAnon::DIRECTION_CONFIGS, by the stair's own direction.
    static constexpr std::array<DirectionConfig, 4> DIRECTION_CONFIGS = {{
        {{0, 0, 1}, {0, 0, -1}, {Direction::East, {-1, 0, 0}, {1, 0, 0}}, {Direction::West, {1, 0, 0}, {-1, 0, 0}}},
        {{-1, 0, 0}, {1, 0, 0}, {Direction::South, {0, 0, -1}, {0, 0, 1}}, {Direction::North, {0, 0, 1}, {0, 0, -1}}},
        {{0, 0, -1}, {0, 0, 1}, {Direction::West, {1, 0, 0}, {-1, 0, 0}}, {Direction::East, {-1, 0, 0}, {1, 0, 0}}},
        {{1, 0, 0}, {-1, 0, 0}, {Direction::North, {0, 0, 1}, {0, 0, -1}}, {Direction::South, {0, 0, -1}, {0, 0, 1}}},
    }};

    struct Stair {
        Direction direction;
        std::int8_t upside_down;
        // The stair with no corner, which the lock test compares.
        std::uint32_t base;
        std::array<std::uint32_t, CORNER_SHAPE_NAMES.size()> shapes;
    };

    // BDS BlockTrait::CornerBehavior::getDirection.
    static Direction getDirection(const bp::CompoundTag &states)
    {
        if (const auto it = states.find("minecraft:cardinal_direction"); it != states.end()) {
            const auto *name = it->second.get_if<bp::StringTag>();
            constexpr std::array<std::string_view, 4> names = {"south", "west", "north", "east"};
            const auto direction = std::ranges::find(names, name != nullptr ? name->value() : "");
            return direction != names.end() ? static_cast<Direction>(direction - names.begin()) : Direction::North;
        }
        if (const auto it = states.find("weirdo_direction"); it != states.end()) {
            const auto *weirdo = it->second.get_if<bp::IntTag>();
            const auto value = weirdo != nullptr ? weirdo->value() : 3;
            return value >= 0 && value <= 2 ? static_cast<Direction>((0x103 >> (8 * value)) & 0xFF) : Direction::North;
        }
        return Direction::North;
    }

    // BDS BlockTrait::CornerBehavior::getIsUpsideDown, -1 for a block with neither state.
    static std::int8_t getIsUpsideDown(const bp::CompoundTag &states)
    {
        if (const auto it = states.find("minecraft:vertical_half"); it != states.end()) {
            const auto *half = it->second.get_if<bp::StringTag>();
            return half != nullptr && half->value() == "top" ? 1 : 0;
        }
        if (const auto it = states.find("upside_down_bit"); it != states.end()) {
            const auto *bit = it->second.get_if<bp::ByteTag>();
            return bit != nullptr && bit->value() != 0 ? 1 : 0;
        }
        return -1;
    }

    // Every shape of every stair the hop gives minecraft:corner, by id.
    static const std::vector<std::pair<std::uint32_t, Stair>> &getStairs()
    {
        static const auto stairs = [] {
            std::vector<std::pair<std::uint32_t, Stair>> result;
            for (const auto &update : Mappings<2168, 2193>::block_states) {
                if (!parseSnbt<bp::CompoundTag>(update.add).contains("minecraft:corner")) {
                    continue;
                }
                const auto &name = update.rename.empty() ? update.name : update.rename;
                forEachBlock(update, [&](const bp::CompoundTag &, const bp::CompoundTag &updated) {
                    Stair stair{getDirection(updated), getIsUpsideDown(updated), 0, {}};
                    for (std::size_t shape = 0; shape < CORNER_SHAPE_NAMES.size(); ++shape) {
                        bp::CompoundTag states = updated;
                        states.insert_or_assign("minecraft:corner",
                                                bp::StringTag{std::string(CORNER_SHAPE_NAMES[shape])});
                        stair.shapes[shape] = computeRawSerializationIdHashForNetwork(name, states);
                    }
                    stair.base = stair.shapes[static_cast<std::size_t>(CornerShape::None)];
                    for (const auto shape : stair.shapes) {
                        result.emplace_back(shape, stair);
                    }
                });
            }
            std::ranges::sort(result, {}, &std::pair<std::uint32_t, Stair>::first);
            return result;
        }();
        return stairs;
    }

    static const Stair *find(std::uint32_t id)
    {
        const auto &stairs = getStairs();
        const auto it = std::ranges::lower_bound(stairs, id, {}, &std::pair<std::uint32_t, Stair>::first);
        return it != stairs.end() && it->first == id ? &it->second : nullptr;
    }
};

/** minecraft:connection_north, east, south and west on fences, panes, bars and tripwire: whether the
 * block on that side accepts the block as a connector.
 * @see BDS ConnectionBlockTraitAnon::StandardConnectionCallback, ViaVersion AbstractFenceConnectionHandler. */
class StandardConnectionHandler {
public:
    template <class Add>
    static void init(Add &&add)
    {
        for (const auto id : getBlocks().getBlockStates()) {
            if (getConnectionId(id)) {
                add(id);
            }
        }
    }

    template <class UserBlockData>
    static std::uint32_t connect(const bp::BlockPos &pos, std::uint32_t id, UserBlockData &&blocks)
    {
        const auto connection_id = getConnectionId(id);
        if (!connection_id) {
            return id;
        }
        std::size_t way = 0;
        for (std::size_t i = 0; i < SIDES.size(); ++i) {
            const auto neighbor = blocks(offsetPos(pos, SIDES[i].offset));
            if (neighbor && canConnect(*connection_id, ConnectionData<2168, 2193>::getBlockConnectionData(*neighbor),
                                       SIDES[i].face)) {
                way |= std::size_t{1} << i;
            }
        }
        return getBlocks().find(id)[way];
    }

private:
    // BDS ConnectionID.
    enum class ConnectionID : std::uint8_t {
        Fence,
        NetherBrickFence,
        Wall,
        ThinFence,
        TripWire,
    };

    struct Side {
        bp::BlockPos offset;
        // BDS Facing::Name of the neighbour's face that looks back at the block.
        std::uint8_t face;
    };

    static constexpr std::array<Side, 4> SIDES = {{{{0, 0, -1}, 3}, {{1, 0, 0}, 4}, {{0, 0, 1}, 2}, {{-1, 0, 0}, 5}}};
    static constexpr std::array<std::string_view, 4> STATES = {
        "minecraft:connection_north", "minecraft:connection_east", "minecraft:connection_south",
        "minecraft:connection_west"};

    // BDS BlockTrait::Connection::getConnectedDirections, for one side.
    static bool canConnect(std::uint8_t connection_id, const BlockData *neighbor, std::uint8_t face)
    {
        if (neighbor == nullptr || !neighbor->connections_from) {
            return connection_id != static_cast<std::uint8_t>(ConnectionID::TripWire);
        }
        return *neighbor->connections_from != 0 && ((neighbor->enabled_directions >> face) & 1) != 0 &&
               ((*neighbor->connections_from >> connection_id) & 1) != 0;
    }

    static std::optional<std::uint8_t> getConnectionId(std::uint32_t id)
    {
        const auto ways = getBlocks().find(id);
        const auto *block = ways.empty() ? nullptr : ConnectionData<2168, 2193>::getBlockConnectionData(ways.front());
        return block != nullptr ? block->connection_id : std::nullopt;
    }

    static const ConnectedBlockStates &getBlocks()
    {
        static const ConnectedBlockStates blocks{Mappings<2168, 2193>::block_states,
                                                 [](const BlockStateUpdate &update) {
                                                     const auto add = parseSnbt<bp::CompoundTag>(update.add);
                                                     return std::ranges::all_of(STATES, [&](std::string_view state) {
                                                         return add.contains(std::string(state));
                                                     });
                                                 },
                                                 {STATES.begin(), STATES.end()},
                                                 {bp::ByteTag{0}, bp::ByteTag{1}}};
        return blocks;
    }
};

/** The handlers of the 26.50 states BDS works out from neighbouring blocks.
 * @see ViaVersion ConnectionData#init. */
template <>
struct ConnectionHandlers<2168, 2193> {
    using type = std::tuple<StairConnectionHandler, StandardConnectionHandler>;
};

} // namespace endweave
