#pragma once

#include <algorithm>
#include <array>
#include <bedrock/protocol/stream.hpp>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace bp = bedrock::protocol;

namespace endweave {

/** One layer of a sub-chunk's blocks, or its biomes: a palette of ids, and the palette index of each
 * of its 4096 blocks. The indices are kept packed as they were read until a block is set. A palette
 * with no ids is BDS's empty storage, which has no blocks, or a sub-chunk whose biomes are the ones
 * below it.
 * @see BDS SubChunkStorage<Block>, SubChunkStorage<Biome>, ViaVersion DataPalette. */
class DataPalette {
public:
    // BDS SubChunkStorageUnit::Type: the bits a palette index can be packed in.
    static constexpr std::array<std::uint8_t, 9> BITS_PER_VALUE = {0, 1, 2, 3, 4, 5, 6, 8, 16};

    /** The 32-bit words that 4096 indices of `bits` bits each are packed in. */
    static constexpr std::size_t wordCount(std::uint8_t bits)
    {
        if (bits == 0) {
            return 0;
        }
        const std::size_t per_word = 32 / bits;
        return (SIZE + per_word - 1) / per_word;
    }

    [[nodiscard]] std::optional<std::uint32_t> idAt(std::uint16_t index) const
    {
        const auto entry = paletteIndexAt(index);
        if (entry >= palette_.size()) {
            return std::nullopt;
        }
        return palette_[entry];
    }

    void setIdAt(std::uint16_t index, std::uint32_t id)
    {
        if (palette_.empty()) {
            return;
        }
        if (values_.empty()) {
            std::vector<std::uint16_t> values(SIZE);
            for (std::size_t i = 0; i < SIZE; ++i) {
                values[i] = paletteIndexAt(static_cast<std::uint16_t>(i));
            }
            values_ = std::move(values);
        }
        auto entry = std::ranges::find(palette_, id);
        if (entry == palette_.end()) {
            entry = palette_.insert(palette_.end(), id);
        }
        values_[index] = static_cast<std::uint16_t>(entry - palette_.begin());
    }

    [[nodiscard]] std::uint32_t idByIndex(std::size_t index) const
    {
        return palette_[index];
    }

    [[nodiscard]] std::uint16_t paletteIndexAt(std::uint16_t index) const
    {
        if (!values_.empty()) {
            return values_[index];
        }
        if (bits_ == 0) {
            return 0;
        }
        const std::size_t per_word = 32 / bits_;
        const auto *word = reinterpret_cast<const unsigned char *>(words_.data()) + index / per_word * 4;
        const std::uint32_t value = word[0] | (word[1] << 8) | (word[2] << 16) | (std::uint32_t{word[3]} << 24);
        return static_cast<std::uint16_t>((value >> (index % per_word * bits_)) & ((1U << bits_) - 1));
    }

    template <class Mapper>
    void replaceIds(Mapper mapper)
    {
        for (auto &id : palette_) {
            id = mapper(id);
        }
    }

    void addId(std::uint32_t id)
    {
        palette_.push_back(id);
    }

    /** Calls `consumer` with the index of every block whose id `predicate` accepts. */
    template <class Predicate, class Consumer>
    void forEachMatchingCoordinate(Predicate predicate, Consumer consumer) const
    {
        std::vector<bool> matching(palette_.size());
        bool any = false;
        for (std::size_t i = 0; i < palette_.size(); ++i) {
            matching[i] = predicate(palette_[i]);
            any = any || matching[i];
        }
        if (!any) {
            return;
        }
        for (std::size_t i = 0; i < SIZE; ++i) {
            const auto index = static_cast<std::uint16_t>(i);
            const auto entry = paletteIndexAt(index);
            if (entry < matching.size() && matching[entry]) {
                consumer(index);
            }
        }
    }

    [[nodiscard]] std::size_t size() const
    {
        return palette_.size();
    }

    /** Takes the indices packed in `words`, `bits` bits each, in place of every block's. */
    void setPaletteIndexes(std::string words, std::uint8_t bits)
    {
        words_ = std::move(words);
        bits_ = bits;
        values_ = {};
    }

    /** The bits each index is packed in: as they were read, or, once a block is set, the fewest the
     * palette needs. */
    [[nodiscard]] std::uint8_t bitsPerValue() const
    {
        if (values_.empty()) {
            return bits_;
        }
        if (palette_.size() <= 1) {
            return 0;
        }
        return *std::ranges::find_if(BITS_PER_VALUE, [&](std::uint8_t bits) {
            return bits != 0 && (std::size_t{1} << bits) >= palette_.size();
        });
    }

    /** The indices packed into words of `bitsPerValue()` bits each. */
    [[nodiscard]] std::string createPackedValues() const
    {
        if (values_.empty()) {
            return words_;
        }
        const auto bits = bitsPerValue();
        std::vector<std::uint32_t> packed(wordCount(bits));
        if (bits != 0) {
            const std::size_t per_word = 32 / bits;
            for (std::size_t i = 0; i < SIZE; ++i) {
                packed[i / per_word] |= std::uint32_t{values_[i]} << (i % per_word * bits);
            }
        }
        std::string words;
        bp::BinaryWriter out{words};
        for (const auto word : packed) {
            out.write<std::uint32_t>(word);
        }
        return words;
    }

    /** Packs blocks that were set back into words, with the fewest bits the palette needs. */
    void compact()
    {
        if (values_.empty()) {
            return;
        }
        const auto bits = bitsPerValue();
        setPaletteIndexes(createPackedValues(), bits);
    }

private:
    static constexpr std::size_t SIZE = 16 * 16 * 16;

    std::vector<std::uint32_t> palette_;
    std::uint8_t bits_ = 0;
    std::string words_;
    std::vector<std::uint16_t> values_;
};

/** A sub-chunk: a palette for each of its block layers, in format 8 or 9.
 * @see BDS SubChunk, ViaVersion ChunkSection. */
class SubChunk {
public:
    SubChunk(std::uint8_t version, std::int32_t y) : version_(version), y_(y) {}

    static constexpr std::uint16_t index(int x, int y, int z)
    {
        return static_cast<std::uint16_t>((x << 8) | (z << 4) | y);
    }

    static constexpr int xFromIndex(std::uint16_t index)
    {
        return (index >> 8) & 0xF;
    }

    static constexpr int yFromIndex(std::uint16_t index)
    {
        return index & 0xF;
    }

    static constexpr int zFromIndex(std::uint16_t index)
    {
        return (index >> 4) & 0xF;
    }

    [[nodiscard]] std::uint8_t getVersion() const
    {
        return version_;
    }

    /** Which one of its column it is, counted in sub-chunks. */
    [[nodiscard]] std::int32_t getY() const
    {
        return y_;
    }

    [[nodiscard]] DataPalette *palette(std::size_t layer)
    {
        return layer < palettes_.size() ? &palettes_[layer] : nullptr;
    }

    [[nodiscard]] const DataPalette *palette(std::size_t layer) const
    {
        return layer < palettes_.size() ? &palettes_[layer] : nullptr;
    }

    [[nodiscard]] std::span<DataPalette> palettes()
    {
        return palettes_;
    }

    [[nodiscard]] std::span<const DataPalette> palettes() const
    {
        return palettes_;
    }

    void addPalette(DataPalette palette)
    {
        palettes_.push_back(std::move(palette));
    }

private:
    std::uint8_t version_;
    std::int32_t y_;
    std::vector<DataPalette> palettes_;
};

/** The sub-chunks of a column that one packet carries, where they are read the biomes of each
 * sub-chunk of its dimension, from the bottom up, and the bytes that follow: border blocks and block
 * actors, which are copied as they are.
 * @see ViaVersion Chunk. */
class Chunk {
public:
    Chunk(std::int32_t x, std::int32_t z, std::vector<SubChunk> sub_chunks, std::vector<DataPalette> biomes,
          std::string rest)
        : x_(x), z_(z), sub_chunks_(std::move(sub_chunks)), biomes_(std::move(biomes)), rest_(std::move(rest))
    {
    }

    [[nodiscard]] std::int32_t getX() const
    {
        return x_;
    }

    [[nodiscard]] std::int32_t getZ() const
    {
        return z_;
    }

    [[nodiscard]] std::vector<SubChunk> &getSubChunks()
    {
        return sub_chunks_;
    }

    [[nodiscard]] const std::vector<SubChunk> &getSubChunks() const
    {
        return sub_chunks_;
    }

    [[nodiscard]] std::span<DataPalette> getBiomes()
    {
        return biomes_;
    }

    [[nodiscard]] std::span<const DataPalette> getBiomes() const
    {
        return biomes_;
    }

    [[nodiscard]] const std::string &getRest() const
    {
        return rest_;
    }

private:
    std::int32_t x_;
    std::int32_t z_;
    std::vector<SubChunk> sub_chunks_;
    std::vector<DataPalette> biomes_;
    std::string rest_;
};

namespace detail {

inline std::unexpected<std::error_code> malformed()
{
    return std::unexpected(std::make_error_code(std::errc::protocol_error));
}

} // namespace detail

/** A layer of a network sub-chunk, or a sub-chunk's biomes: the bits each palette index is packed in,
 * the packed indices, and the palette.
 * @see ViaVersion PaletteType1_18. */
class PaletteType {
public:
    static std::expected<DataPalette, std::error_code> read(bp::BinaryReader &in)
    {
        const auto header = in.read<std::uint8_t>();
        if (!header || (*header & 1) == 0) {
            return detail::malformed();
        }
        DataPalette palette;
        const std::uint8_t bits = *header >> 1;
        if (bits == EMPTY) {
            return palette;
        }
        if (std::ranges::find(DataPalette::BITS_PER_VALUE, bits) == DataPalette::BITS_PER_VALUE.end()) {
            return detail::malformed();
        }
        const std::size_t bytes = DataPalette::wordCount(bits) * 4;
        if (bytes > in.getUnreadLength()) {
            return detail::malformed();
        }
        palette.setPaletteIndexes(std::string(in.getView().substr(in.getReadPointer(), bytes)), bits);
        in.setReadPointer(in.getReadPointer() + bytes);

        std::int32_t size = 1;
        if (bits != 0) {
            const auto palette_size = in.readVarInt<std::int32_t>();
            if (!palette_size || *palette_size <= 0 || *palette_size > (1 << bits)) {
                return detail::malformed();
            }
            size = *palette_size;
        }
        for (std::int32_t i = 0; i < size; ++i) {
            const auto id = in.readVarInt<std::int32_t>();
            if (!id) {
                return detail::malformed();
            }
            palette.addId(static_cast<std::uint32_t>(*id));
        }
        return palette;
    }

    static void write(bp::BinaryWriter &out, const DataPalette &palette)
    {
        if (palette.size() == 0) {
            out.write<std::uint8_t>((EMPTY << 1) | 1);
            return;
        }
        const auto bits = palette.bitsPerValue();
        out.write<std::uint8_t>((bits << 1) | 1);
        out.writeRawBytes(palette.createPackedValues());
        if (bits != 0) {
            out.writeVarInt<std::int32_t>(static_cast<std::int32_t>(palette.size()));
        }
        for (std::size_t i = 0; i < palette.size(); ++i) {
            out.writeVarInt<std::int32_t>(static_cast<std::int32_t>(palette.idByIndex(i)));
        }
    }

private:
    // BDS SubChunkStorageUnit::Type::Empty.
    static constexpr std::uint8_t EMPTY = 0x7F;
};

/** A network sub-chunk in format 8 or 9: a layer for each block layer.
 * @see ViaVersion ChunkSectionType1_18. */
class SubChunkType {
public:
    /** Format 8 does not say which one of its column the sub-chunk is, so it is `y`. */
    static std::expected<SubChunk, std::error_code> read(bp::BinaryReader &in, std::int32_t y)
    {
        const auto version = in.read<std::uint8_t>();
        const auto count = in.read<std::uint8_t>();
        if (!version || !count || (*version != 8 && *version != 9)) {
            return detail::malformed();
        }
        if (*version == 9) {
            const auto y_index = in.read<std::uint8_t>();
            if (!y_index) {
                return detail::malformed();
            }
            y = static_cast<std::int8_t>(*y_index);
        }
        SubChunk sub_chunk{*version, y};
        for (std::uint8_t i = 0; i < *count; ++i) {
            auto palette = PaletteType::read(in);
            if (!palette) {
                return std::unexpected(palette.error());
            }
            sub_chunk.addPalette(std::move(*palette));
        }
        return sub_chunk;
    }

    static void write(bp::BinaryWriter &out, const SubChunk &sub_chunk)
    {
        out.write<std::uint8_t>(sub_chunk.getVersion());
        out.write<std::uint8_t>(static_cast<std::uint8_t>(sub_chunk.palettes().size()));
        if (sub_chunk.getVersion() == 9) {
            out.write<std::uint8_t>(static_cast<std::uint8_t>(sub_chunk.getY()));
        }
        for (const auto &palette : sub_chunk.palettes()) {
            PaletteType::write(out, palette);
        }
    }
};

/** The `count` network sub-chunks of a column, its `biome_count` biome storages, and the bytes after
 * them.
 * @see ViaVersion ChunkType1_18. */
class ChunkType {
public:
    /** Reads the sub-chunks of the column at `x`, `z`, counted in sub-chunks, then its biomes, then
     * keeps the rest of `in`. Sub-chunks in format 8 count up from `y`. */
    static std::expected<Chunk, std::error_code> read(bp::BinaryReader &in, std::int32_t x, std::int32_t y,
                                                      std::int32_t z, std::size_t count, std::size_t biome_count)
    {
        std::vector<SubChunk> sub_chunks;
        for (std::size_t i = 0; i < count; ++i) {
            auto sub_chunk = SubChunkType::read(in, y + static_cast<std::int32_t>(i));
            if (!sub_chunk) {
                return std::unexpected(sub_chunk.error());
            }
            sub_chunks.push_back(std::move(*sub_chunk));
        }
        std::vector<DataPalette> biomes;
        for (std::size_t i = 0; i < biome_count; ++i) {
            auto biome = PaletteType::read(in);
            if (!biome) {
                return std::unexpected(biome.error());
            }
            biomes.push_back(std::move(*biome));
        }
        std::string rest(in.getView().substr(in.getReadPointer()));
        in.setReadPointer(in.getReadPointer() + rest.size());
        return Chunk{x, z, std::move(sub_chunks), std::move(biomes), std::move(rest)};
    }

    static void write(bp::BinaryWriter &out, const Chunk &chunk)
    {
        for (const auto &sub_chunk : chunk.getSubChunks()) {
            SubChunkType::write(out, sub_chunk);
        }
        for (const auto &biome : chunk.getBiomes()) {
            PaletteType::write(out, biome);
        }
        out.writeRawBytes(chunk.getRest());
    }
};

} // namespace endweave
