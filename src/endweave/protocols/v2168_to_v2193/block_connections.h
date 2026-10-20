#pragma once

#include "block_states.h"
#include "connection_rules.h"
#include "endweave/protocol/block.h"

#include <algorithm>
#include <array>
#include <bedrock/protocol/common.h>
#include <bedrock/protocol/nbt.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace endweave {

/** minecraft:corner, the shape a stair takes from the stairs in front of and behind it.
 * @see BDS CornerBehaviorAnon::CornerConnectionCallback, ViaVersion StairConnectionHandler. */
class StairConnectionHandler {
public:
    static bool connects(std::uint32_t id)
    {
        return find(id) != nullptr;
    }

    template <class Get>
    static std::optional<std::uint32_t> connect(const bp::BlockPos &pos, std::uint32_t id, Get &&get)
    {
        const auto *self = find(id);
        if (self == nullptr) {
            return std::nullopt;
        }
        const auto stair_at = [&](const bp::BlockPos &offset) -> const Stair * {
            const auto block = get(bp::BlockPos{pos.x + offset.x, pos.y + offset.y, pos.z + offset.z});
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
    static const Stair *find(std::uint32_t id)
    {
        static const auto stairs = [] {
            std::vector<std::pair<std::uint32_t, Stair>> result;
            for (const auto &update : getBlockStateUpdates<2168, 2193>()) {
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
        const auto it = std::ranges::lower_bound(stairs, id, {}, &std::pair<std::uint32_t, Stair>::first);
        return it != stairs.end() && it->first == id ? &it->second : nullptr;
    }
};

/** minecraft:connection_north, east, south and west on fences, panes, bars and tripwire: whether the
 * block on that side accepts the block as a connector.
 * @see BDS ConnectionBlockTraitAnon::StandardConnectionCallback, ViaVersion AbstractFenceConnectionHandler. */
class StandardConnectionHandler {
public:
    static bool connects(std::uint32_t id)
    {
        return findBlock(id) != nullptr;
    }

    template <class Get>
    static std::optional<std::uint32_t> connect(const bp::BlockPos &pos, std::uint32_t id, Get &&get)
    {
        const auto *self = findBlock(id);
        if (self == nullptr) {
            return std::nullopt;
        }
        std::size_t connections = 0;
        for (std::size_t i = 0; i < SIDES.size(); ++i) {
            const auto &[offset, face, state] = SIDES[i];
            const auto neighbor = get(bp::BlockPos{pos.x + offset.x, pos.y + offset.y, pos.z + offset.z});
            if (neighbor && canConnect(self->connection_id, *neighbor, face)) {
                connections |= std::size_t{1} << i;
            }
        }
        return self->connections[connections];
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
        std::string_view state;
    };

    static constexpr std::array<Side, 4> SIDES = {{
        {{0, 0, -1}, 3, "minecraft:connection_north"},
        {{1, 0, 0}, 4, "minecraft:connection_east"},
        {{0, 0, 1}, 2, "minecraft:connection_south"},
        {{-1, 0, 0}, 5, "minecraft:connection_west"},
    }};

    struct Rule {
        std::optional<std::uint8_t> connections_from;
        std::uint8_t enabled_directions;
        std::optional<std::uint8_t> connection_id;
    };

    struct Block {
        std::uint8_t connection_id;
        std::array<std::uint32_t, std::size_t{1} << SIDES.size()> connections;
    };

    // BDS BlockTrait::Connection::getConnectedDirections, for one side.
    static bool canConnect(std::uint8_t connection_id, std::uint32_t neighbor, std::uint8_t face)
    {
        const auto *rule = findRule(neighbor);
        if (rule == nullptr || !rule->connections_from) {
            return connection_id != static_cast<std::uint8_t>(ConnectionID::TripWire);
        }
        return *rule->connections_from != 0 && ((rule->enabled_directions >> face) & 1) != 0 &&
               ((*rule->connections_from >> connection_id) & 1) != 0;
    }

    static const Rule *findRule(std::uint32_t id)
    {
        static const auto rules = [] {
            std::vector<std::pair<std::uint32_t, Rule>> result;
            for (const auto &rule : getBlockConnectionRules<2168, 2193>()) {
                forEachState(parseSnbt<bp::CompoundTag>(rule.states), [&](const bp::CompoundTag &states) {
                    result.emplace_back(computeRawSerializationIdHashForNetwork(rule.name, states),
                                        Rule{rule.connections_from, rule.enabled_directions, rule.connection_id});
                });
            }
            std::ranges::sort(result, {}, &std::pair<std::uint32_t, Rule>::first);
            return result;
        }();
        const auto it = std::ranges::lower_bound(rules, id, {}, &std::pair<std::uint32_t, Rule>::first);
        return it != rules.end() && it->first == id ? &it->second : nullptr;
    }

    // Every block the hop gives the four minecraft:connection_* states, by id.
    static const Block *findBlock(std::uint32_t id)
    {
        static const auto blocks = [] {
            std::vector<std::pair<std::uint32_t, Block>> result;
            for (const auto &update : getBlockStateUpdates<2168, 2193>()) {
                const auto add = parseSnbt<bp::CompoundTag>(update.add);
                if (!std::ranges::all_of(SIDES, [&](const Side &side) {
                        return add.contains(std::string(side.state));
                    })) {
                    continue;
                }
                const auto &name = update.rename.empty() ? update.name : update.rename;
                forEachBlock(update, [&](const bp::CompoundTag &, const bp::CompoundTag &updated) {
                    const auto *rule = findRule(computeRawSerializationIdHashForNetwork(name, updated));
                    if (rule == nullptr || !rule->connection_id) {
                        return;
                    }
                    Block block{*rule->connection_id, {}};
                    for (std::size_t connections = 0; connections < block.connections.size(); ++connections) {
                        bp::CompoundTag states = updated;
                        for (std::size_t i = 0; i < SIDES.size(); ++i) {
                            states.insert_or_assign(std::string(SIDES[i].state),
                                                    bp::ByteTag{static_cast<std::int8_t>((connections >> i) & 1)});
                        }
                        block.connections[connections] = computeRawSerializationIdHashForNetwork(name, states);
                    }
                    for (const auto connected : block.connections) {
                        result.emplace_back(connected, block);
                    }
                });
            }
            std::ranges::sort(result, {}, &std::pair<std::uint32_t, Block>::first);
            return result;
        }();
        const auto it = std::ranges::lower_bound(blocks, id, {}, &std::pair<std::uint32_t, Block>::first);
        return it != blocks.end() && it->first == id ? &it->second : nullptr;
    }
};

/** The 26.50 states BDS works out from neighbouring blocks.
 * @see ViaVersion ConnectionData. */
template <>
struct ConnectionData<2168, 2193> {
    static bool connects(std::uint32_t id)
    {
        return StairConnectionHandler::connects(id) || StandardConnectionHandler::connects(id);
    }

    template <class Get>
    static std::optional<std::uint32_t> connect(const bp::BlockPos &pos, std::uint32_t id, Get &&get)
    {
        if (auto stair = StairConnectionHandler::connect(pos, id, get)) {
            return stair;
        }
        return StandardConnectionHandler::connect(pos, id, get);
    }
};

} // namespace endweave
