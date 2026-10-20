#pragma once

#include "block_connections.h"
#include "endweave/protocol/block_connection.h"
#include "mappings.h"

#include <array>
#include <bedrock/protocol/common.h>
#include <bedrock/protocol/nbt.hpp>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>

namespace endweave {

/** minecraft:connection_* on a chorus plant: whether the block on that side is chorus, or end stone
 * below it.
 * @see BDS ConnectionBlockTraitAnon::FunctionConnectionCallback, ViaVersion ChorusPlantConnectionHandler. */
class ChorusPlantConnectionHandler {
public:
    template <class Add>
    static void init(Add &&add)
    {
        for (const auto id : getBlocks().getBlockStates()) {
            add(id);
        }
    }

    template <class UserBlockData>
    static std::uint32_t connect(const bp::BlockPos &pos, std::uint32_t id, UserBlockData &&blocks)
    {
        const auto ways = getBlocks().find(id);
        if (ways.empty()) {
            return id;
        }
        std::size_t way = 0;
        for (std::size_t i = 0; i < OFFSETS.size(); ++i) {
            const auto neighbor = blocks(offsetPos(pos, OFFSETS[i]));
            const auto *block = neighbor ? ConnectionData<2193, 2225>::getBlockConnectionData(*neighbor) : nullptr;
            const auto name = block != nullptr ? block->name : std::string_view{};
            if (name == "minecraft:chorus_plant" || name == "minecraft:chorus_flower" ||
                (OFFSETS[i].y < 0 && name == "minecraft:end_stone")) {
                way |= std::size_t{1} << i;
            }
        }
        return ways[way];
    }

private:
    static constexpr std::array<bp::BlockPos, 6> OFFSETS = {
        {{0, 0, -1}, {1, 0, 0}, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}}};

    static const ConnectedBlockStates &getBlocks()
    {
        static const ConnectedBlockStates blocks{Mappings<2193, 2225>::block_states,
                                                 [](const BlockStateUpdate &update) {
                                                     return update.name == "minecraft:chorus_plant";
                                                 },
                                                 {"minecraft:connection_north", "minecraft:connection_east",
                                                  "minecraft:connection_south", "minecraft:connection_west",
                                                  "minecraft:connection_up", "minecraft:connection_down"},
                                                 {bp::ByteTag{0}, bp::ByteTag{1}}};
        return blocks;
    }
};

/** minecraft:connection_* on fire: whether the block on that side burns.
 * @see BDS ConnectionBlockTraitAnon::FunctionConnectionCallback, ViaVersion FireConnectionHandler. */
class FireConnectionHandler {
public:
    template <class Add>
    static void init(Add &&add)
    {
        for (const auto id : getBlocks().getBlockStates()) {
            add(id);
        }
    }

    template <class UserBlockData>
    static std::uint32_t connect(const bp::BlockPos &pos, std::uint32_t id, UserBlockData &&blocks)
    {
        const auto ways = getBlocks().find(id);
        if (ways.empty()) {
            return id;
        }
        std::size_t way = 0;
        for (std::size_t i = 0; i < OFFSETS.size(); ++i) {
            const auto neighbor = blocks(offsetPos(pos, OFFSETS[i]));
            const auto *block = neighbor ? ConnectionData<2193, 2225>::getBlockConnectionData(*neighbor) : nullptr;
            if (block != nullptr && block->flame_odds != 0) {
                way |= std::size_t{1} << i;
            }
        }
        return ways[way];
    }

private:
    static constexpr std::array<bp::BlockPos, 5> OFFSETS = {{{0, 0, -1}, {1, 0, 0}, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}}};

    static const ConnectedBlockStates &getBlocks()
    {
        static const ConnectedBlockStates blocks{Mappings<2193, 2225>::block_states,
                                                 [](const BlockStateUpdate &update) {
                                                     return update.name == "minecraft:fire";
                                                 },
                                                 {"minecraft:connection_north", "minecraft:connection_east",
                                                  "minecraft:connection_south", "minecraft:connection_west",
                                                  "minecraft:connection_up"},
                                                 {bp::ByteTag{0}, bp::ByteTag{1}}};
        return blocks;
    }
};

/** redstone_north, east, south and west on redstone wire: none, side, or up where the wire climbs the
 * block on that side. BDS also asks whether the wire on top of that block can survive there, which a
 * wire the server keeps there can.
 * @see BDS RedStoneWireBlock, ViaVersion RedstoneConnectionHandler. */
class RedstoneConnectionHandler {
public:
    template <class Add>
    static void init(Add &&add)
    {
        for (const auto id : getBlocks().getBlockStates()) {
            add(id);
        }
    }

    template <class UserBlockData>
    static std::uint32_t connect(const bp::BlockPos &pos, std::uint32_t id, UserBlockData &&blocks)
    {
        const auto ways = getBlocks().find(id);
        if (ways.empty()) {
            return id;
        }
        const auto at = [&](const bp::BlockPos &offset) -> const BlockData * {
            const auto neighbor = blocks(offsetPos(pos, offset));
            return neighbor ? ConnectionData<2193, 2225>::getBlockConnectionData(*neighbor) : nullptr;
        };
        const auto none = [](RedstoneSide side) {
            return side == RedstoneSide::None;
        };
        const auto south = getConnection(at, Direction::South);
        const auto west = getConnection(at, Direction::West);
        const auto north = getConnection(at, Direction::North);
        const auto east = getConnection(at, Direction::East);
        const std::array sides = {
            none(north) && none(west) && none(east) ? RedstoneSide::Side : north,
            none(east) && none(north) && none(south) ? RedstoneSide::Side : east,
            none(south) && none(west) && none(east) ? RedstoneSide::Side : south,
            none(west) && none(north) && none(south) ? RedstoneSide::Side : west,
        };
        std::size_t way = 0;
        for (auto i = sides.size(); i-- > 0;) {
            way = way * 3 + static_cast<std::size_t>(sides[i]);
        }
        return ways[way];
    }

private:
    // BDS Direction::Type.
    enum class Direction : std::uint8_t {
        South,
        West,
        North,
        East,
    };

    // BDS RedstoneSide, named as EnumStateMapper<RedstoneSide> names it.
    enum class RedstoneSide : std::uint8_t {
        None,
        Side,
        Up,
    };

    static constexpr std::array<bp::BlockPos, 4> OFFSETS = {{{0, 0, 1}, {-1, 0, 0}, {0, 0, -1}, {1, 0, 0}}};
    // BDS Direction::DIRECTION_FACING and Direction::DIRECTION_OPPOSITE.
    static constexpr std::array<std::uint8_t, 4> DIRECTION_FACING = {3, 4, 2, 5};
    static constexpr std::array<std::uint8_t, 4> DIRECTION_OPPOSITE = {2, 3, 0, 1};

    static bool isRedstoneWire(const BlockData *block)
    {
        return block != nullptr && block->name == "minecraft:redstone_wire";
    }

    static bool isPlainConductor(const BlockData *block)
    {
        return block != nullptr && block->redstone_conductor && !block->redstone_producer_faces &&
               !block->signal_source && !block->consumer_component;
    }

    // BDS RedStoneWireBlock::getConnection.
    template <class At>
    static RedstoneSide getConnection(At &&at, Direction direction)
    {
        const auto index = static_cast<std::size_t>(direction);
        const auto &side = OFFSETS[index];
        const auto face = DIRECTION_FACING[DIRECTION_OPPOSITE[index]];
        const auto *block = at(side);
        if (!isPlainConductor(at({0, 1, 0})) && isRedstoneWire(at({side.x, 1, side.z}))) {
            return block != nullptr && ((block->full_support_faces >> face) & 1) != 0 ? RedstoneSide::Up
                                                                                      : RedstoneSide::Side;
        }
        if (block != nullptr && block->redstone_producer_faces &&
            ((*block->redstone_producer_faces >> face) & 1) != 0) {
            return RedstoneSide::Side;
        }
        if (block != nullptr && ((block->should_connect_to_redstone >> index) & 1) != 0) {
            return RedstoneSide::Side;
        }
        if (isPlainConductor(block)) {
            return RedstoneSide::None;
        }
        return isRedstoneWire(at({side.x, -1, side.z})) ? RedstoneSide::Side : RedstoneSide::None;
    }

    static const ConnectedBlockStates &getBlocks()
    {
        static const ConnectedBlockStates blocks{Mappings<2193, 2225>::block_states,
                                                 [](const BlockStateUpdate &update) {
                                                     return update.name == "minecraft:redstone_wire";
                                                 },
                                                 {"redstone_north", "redstone_east", "redstone_south", "redstone_west"},
                                                 {bp::StringTag{"none"}, bp::StringTag{"side"}, bp::StringTag{"up"}}};
        return blocks;
    }
};

/** The handlers of the 26.60 states BDS works out from neighbouring blocks.
 * @see ViaVersion ConnectionData#init. */
template <>
struct ConnectionHandlers<2193, 2225> {
    using type = std::tuple<ChorusPlantConnectionHandler, FireConnectionHandler, RedstoneConnectionHandler>;
};

} // namespace endweave
