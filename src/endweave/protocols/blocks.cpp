#include "blocks.h"

#include <algorithm>
#include <bedrock/protocol/serializer.hpp>
#include <bedrock/protocol/stream.hpp>
#include <span>
#include <unordered_map>

namespace bp = bedrock::protocol;
namespace endweave::blocks {
namespace {
struct BlockMapping {
    std::uint32_t from;
    std::uint32_t to;
};
#include "block_mappings.inc"
struct BlockShape {
    std::uint32_t id;
    std::uint8_t kind, facing, top, faces;
};
struct ConnectedBlock {
    std::uint32_t id;
    std::array<std::uint32_t, 16> variants;
};
#include "block_connections.inc"

const BlockShape &shape(std::uint32_t id)
{
    static constexpr BlockShape empty{};
    auto it = std::lower_bound(kShapes.begin(), kShapes.end(), id, [](const BlockShape &entry, std::uint32_t key) {
        return entry.id < key;
    });
    return it != kShapes.end() && it->id == id ? *it : empty;
}

std::uint32_t at(WorldContext &world, int x, int y, int z)
{
    const std::array<int, 4> key{world.dimension, x, y, z};
    auto it = world.cache.find(key);
    if (it != world.cache.end()) {
        return it->second;
    }
    const auto id = world.lookup(world.dimension, x, y, z);
    world.cache.emplace(key, id);
    return id;
}

constexpr std::array<int, 4> dx{0, 1, 0, -1}, dz{-1, 0, 1, 0};
} // namespace

std::uint32_t mapId(std::uint32_t id, bool to_new)
{
    const std::span<const BlockMapping> mappings =
        to_new ? std::span<const BlockMapping>(kBlocksTo2193) : std::span<const BlockMapping>(kBlocksTo2168);
    auto it = std::lower_bound(mappings.begin(), mappings.end(), id, [](const BlockMapping &entry, std::uint32_t key) {
        return entry.from < key;
    });
    return it != mappings.end() && it->from == id ? it->to : id;
}

std::uint32_t connectedId(std::uint32_t id, int x, int y, int z, WorldContext &world)
{
    auto entry =
        std::lower_bound(kConnections.begin(), kConnections.end(), id, [](const ConnectedBlock &b, std::uint32_t key) {
            return b.id < key;
        });
    if (entry == kConnections.end() || entry->id != id) {
        return mapId(id, true);
    }
    const auto &self = shape(id);
    auto neighbor = [&](int direction) -> const BlockShape & {
        return shape(at(world, x + dx[direction], y, z + dz[direction]));
    };
    unsigned variant = 0;
    if (self.kind == 1) {
        auto canTurn = [&](int direction) {
            const auto &other = neighbor(direction);
            return other.kind != 1 || other.facing != self.facing || other.top != self.top;
        };
        const auto &front = neighbor(self.facing);
        const auto &back = neighbor((self.facing + 2) % 4);
        if (front.kind == 1 && front.top == self.top && (front.facing % 2 != self.facing % 2) &&
            canTurn((front.facing + 2) % 4)) {
            variant = front.facing == (self.facing + 3) % 4 ? 3 : 4;
        }
        else if (back.kind == 1 && back.top == self.top && (back.facing % 2 != self.facing % 2) &&
                 canTurn(back.facing)) {
            variant = back.facing == (self.facing + 3) % 4 ? 1 : 2;
        }
    }
    else {
        for (int direction = 0; direction < 4; ++direction) {
            const auto &other = neighbor(direction);
            bool connected = false;
            if (self.kind == 5) {
                connected = other.kind == 5 || (other.kind == 7 && other.facing == (direction + 2) % 4);
            }
            else {
                connected = (other.faces & (1 << ((direction + 2) % 4))) != 0;
                if (self.kind == 2 || self.kind == 3) {
                    connected |= other.kind == self.kind || (other.kind == 6 && other.facing % 2 != direction % 2);
                }
                else if (self.kind == 4) {
                    connected |= other.kind == 4;
                }
            }
            if (connected) {
                variant |= 1 << direction;
            }
        }
    }
    return entry->variants[variant];
}

std::vector<std::string> neighborUpdates(WorldContext &world)
{
    std::set<std::array<int, 3>> positions;
    for (const auto &pos : world.changed) {
        // Stairs also test a diagonal neighbor when selecting a corner.
        for (int x = -2; x <= 2; ++x) {
            for (int z = -2; z <= 2; ++z) {
                if (std::abs(x) + std::abs(z) <= 2 && (x != 0 || z != 0)) {
                    positions.insert({pos[0] + x, pos[1], pos[2] + z});
                }
            }
        }
    }
    std::vector<std::string> result;
    for (const auto &pos : positions) {
        const auto id = at(world, pos[0], pos[1], pos[2]);
        const auto kind = shape(id).kind;
        if (kind == 0 || kind > 5) {
            continue;
        }
        bp::UpdateBlockPacket_<2193> packet;
        packet.pos.x = pos[0];
        packet.pos.y = pos[1];
        packet.pos.z = pos[2];
        packet.runtime_id = connectedId(id, pos[0], pos[1], pos[2], world);
        packet.update_flags = 3;
        result.emplace_back();
        bp::BinaryWriter writer{result.back()};
        bp::serialize(writer, packet);
    }
    return result;
}

std::expected<void, std::error_code> rewriteSubChunks(std::string &data, std::uint32_t count, bool to_new,
                                                      WorldContext *world, int x, int y, int z)
{
    // Each storage consists of packed block indices followed by a signed-varint hash palette.
    // Only the palette changes; block indices, extra layers, biomes and block-actor NBT stay intact.
    bp::BinaryReader in{data};
    std::string result;
    result.reserve(data.size());
    bp::BinaryWriter out{result};
    auto fail = [] {
        return std::unexpected(std::make_error_code(std::errc::protocol_error));
    };
    for (std::uint32_t chunk = 0; chunk < count; ++chunk) {
        auto version = in.read<std::uint8_t>();
        auto layers = in.read<std::uint8_t>();
        if (!version || !layers || (*version != 8 && *version != 9)) {
            return fail();
        }
        out.write<std::uint8_t>(*version);
        out.write<std::uint8_t>(*layers);
        if (*version == 9) {
            auto index = in.read<std::uint8_t>();
            if (!index) {
                return fail();
            }
            out.write<std::uint8_t>(*index);
            y = static_cast<std::int8_t>(*index) * 16;
        }
        for (unsigned layer = 0; layer < *layers; ++layer) {
            auto header = in.read<std::uint8_t>();
            if (!header) {
                return fail();
            }
            unsigned bits = *header >> 1;
            if (bits == 127) {
                out.write<std::uint8_t>(*header);
                continue;
            }
            if ((*header & 1) == 0 || (bits != 0 && bits != 1 && bits != 2 && bits != 3 && bits != 4 && bits != 5 &&
                                       bits != 6 && bits != 8 && bits != 16)) {
                return fail();
            }
            const std::size_t words = bits == 0 ? 0 : (4096 + (32 / bits) - 1) / (32 / bits);
            const auto bytes = words * 4;
            if (bytes > in.getUnreadLength()) {
                return fail();
            }
            std::vector<std::uint32_t> packed(words);
            for (auto &word : packed) {
                word = *in.read<std::uint32_t>();
            }
            std::int32_t size = 1;
            if (bits != 0) {
                auto palette_size = in.readVarInt<std::int32_t>();
                if (!palette_size || *palette_size <= 0 || *palette_size > (1 << bits)) {
                    return fail();
                }
                size = *palette_size;
            }
            std::vector<std::uint32_t> palette;
            bool connections = false;
            for (std::int32_t i = 0; i < size; ++i) {
                auto id = in.readVarInt<std::int32_t>();
                if (!id) {
                    return fail();
                }
                palette.push_back(static_cast<std::uint32_t>(*id));
                const auto kind = shape(palette.back()).kind;
                connections |= kind >= 1 && kind <= 5;
            }
            if (connections && world && to_new && layer == 0) {
                std::array<std::uint32_t, 4096> indices;
                std::vector<std::uint32_t> mapped;
                std::unordered_map<std::uint32_t, std::uint32_t> index_for;
                for (unsigned i = 0; i < indices.size(); ++i) {
                    auto p =
                        bits == 0 ? 0 : (packed[i / (32 / bits)] >> ((i % (32 / bits)) * bits)) & ((1u << bits) - 1);
                    if (p >= palette.size()) {
                        return fail();
                    }
                    auto id = connectedId(palette[p], x + (i >> 8), y + (i & 15), z + ((i >> 4) & 15), *world);
                    auto [it, inserted] = index_for.emplace(id, static_cast<std::uint32_t>(mapped.size()));
                    if (inserted) {
                        mapped.push_back(id);
                    }
                    indices[i] = it->second;
                }
                for (unsigned candidate : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 8u, 16u}) {
                    if (candidate >= bits && mapped.size() <= (1u << candidate)) {
                        bits = candidate;
                        break;
                    }
                }
                packed.assign(bits == 0 ? 0 : (4096 + 32 / bits - 1) / (32 / bits), 0);
                if (bits != 0) {
                    for (unsigned i = 0; i < indices.size(); ++i) {
                        packed[i / (32 / bits)] |= indices[i] << ((i % (32 / bits)) * bits);
                    }
                }
                palette = std::move(mapped);
            }
            else {
                for (auto &id : palette) {
                    id = mapId(id, to_new);
                }
            }
            out.write<std::uint8_t>((bits << 1) | 1);
            for (auto word : packed) {
                out.write<std::uint32_t>(word);
            }
            if (bits != 0) {
                out.writeVarInt<std::int32_t>(palette.size());
            }
            for (auto id : palette) {
                out.writeVarInt<std::int32_t>(id);
            }
        }
        y += 16;
    }
    out.writeRawBytes(in.getView().substr(in.getReadPointer()));
    data = std::move(result);
    return {};
}
} // namespace endweave::blocks
