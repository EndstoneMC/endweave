#pragma once

#include <algorithm>
#include <array>
#include <bedrock/protocol/stream.hpp>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace bp = bedrock::protocol;

namespace endweave {

/** One layer of a sub-chunk's blocks: a palette of block ids, and the palette index of each of its
 * 4096 blocks, packed into 32-bit words in x, z, y order. The words are kept as they were read
 * until a block is set.
 * @see BDS SubChunkStorage<Block>, ViaVersion DataPalette. */
class SubChunkStorage {
public:
    static constexpr std::size_t SIZE = 4096;

    static constexpr std::uint16_t getIndex(int x, int y, int z)
    {
        return static_cast<std::uint16_t>((x << 8) | (z << 4) | y);
    }

    static std::expected<SubChunkStorage, std::error_code> read(bp::BinaryReader &in)
    {
        const auto header = in.read<std::uint8_t>();
        if (!header || (*header & 1) == 0) {
            return malformed();
        }
        SubChunkStorage storage;
        storage.bits_ = *header >> 1;
        if (storage.bits_ == EMPTY) {
            return storage;
        }
        if (std::ranges::find(TYPES, storage.bits_) == TYPES.end()) {
            return malformed();
        }
        const std::size_t bytes = getWordCount(storage.bits_) * 4;
        if (bytes > in.getUnreadLength()) {
            return malformed();
        }
        storage.words_ = in.getView().substr(in.getReadPointer(), bytes);
        in.setReadPointer(in.getReadPointer() + bytes);

        std::int32_t size = 1;
        if (storage.bits_ != 0) {
            const auto palette_size = in.readVarInt<std::int32_t>();
            if (!palette_size || *palette_size <= 0 || *palette_size > (1 << storage.bits_)) {
                return malformed();
            }
            size = *palette_size;
        }
        storage.palette_.reserve(size);
        for (std::int32_t i = 0; i < size; ++i) {
            const auto id = in.readVarInt<std::int32_t>();
            if (!id) {
                return malformed();
            }
            storage.palette_.push_back(static_cast<std::uint32_t>(*id));
        }
        return storage;
    }

    void write(bp::BinaryWriter &out) const
    {
        std::uint8_t bits = bits_;
        if (!blocks_.empty()) {
            bits = palette_.size() <= 1 ? 0 : *std::ranges::find_if(TYPES, [&](std::uint8_t type) {
                return type != 0 && (std::size_t{1} << type) >= palette_.size();
            });
        }
        out.write<std::uint8_t>((bits << 1) | 1);
        if (bits == EMPTY) {
            return;
        }
        if (blocks_.empty()) {
            out.writeRawBytes(words_);
        }
        else if (bits != 0) {
            const std::size_t per_word = 32 / bits;
            std::vector<std::uint32_t> words(getWordCount(bits));
            for (std::size_t i = 0; i < SIZE; ++i) {
                words[i / per_word] |= std::uint32_t{blocks_[i]} << (i % per_word * bits);
            }
            for (const auto word : words) {
                out.write<std::uint32_t>(word);
            }
        }
        if (bits != 0) {
            out.writeVarInt<std::int32_t>(static_cast<std::int32_t>(palette_.size()));
        }
        for (const auto id : palette_) {
            out.writeVarInt<std::int32_t>(static_cast<std::int32_t>(id));
        }
    }

    [[nodiscard]] std::vector<std::uint32_t> &getPalette()
    {
        return palette_;
    }

    [[nodiscard]] std::optional<std::uint32_t> getElement(std::uint16_t index) const
    {
        if (bits_ == EMPTY) {
            return std::nullopt;
        }
        const auto entry = getPaletteIndex(index);
        if (entry >= palette_.size()) {
            return std::nullopt;
        }
        return palette_[entry];
    }

    void setElement(std::uint16_t index, std::uint32_t id)
    {
        if (bits_ == EMPTY) {
            return;
        }
        if (blocks_.empty()) {
            std::vector<std::uint16_t> blocks(SIZE);
            for (std::size_t i = 0; i < SIZE; ++i) {
                blocks[i] = getPaletteIndex(static_cast<std::uint16_t>(i));
            }
            blocks_ = std::move(blocks);
        }
        auto entry = std::ranges::find(palette_, id);
        if (entry == palette_.end()) {
            entry = palette_.insert(palette_.end(), id);
        }
        blocks_[index] = static_cast<std::uint16_t>(entry - palette_.begin());
    }

private:
    // BDS SubChunkStorageUnit::Type: the bits each block's palette index takes, and Empty.
    static constexpr std::array<std::uint8_t, 9> TYPES = {0, 1, 2, 3, 4, 5, 6, 8, 16};
    static constexpr std::uint8_t EMPTY = 0x7F;

    static std::unexpected<std::error_code> malformed()
    {
        return std::unexpected(std::make_error_code(std::errc::protocol_error));
    }

    static constexpr std::size_t getWordCount(std::uint8_t bits)
    {
        if (bits == 0) {
            return 0;
        }
        const std::size_t per_word = 32 / bits;
        return (SIZE + per_word - 1) / per_word;
    }

    [[nodiscard]] std::uint16_t getPaletteIndex(std::uint16_t index) const
    {
        if (!blocks_.empty()) {
            return blocks_[index];
        }
        if (bits_ == 0) {
            return 0;
        }
        const std::size_t per_word = 32 / bits_;
        const auto *word = reinterpret_cast<const unsigned char *>(words_.data()) + index / per_word * 4;
        const std::uint32_t value = word[0] | (word[1] << 8) | (word[2] << 16) | (std::uint32_t{word[3]} << 24);
        return static_cast<std::uint16_t>((value >> (index % per_word * bits_)) & ((1U << bits_) - 1));
    }

    std::uint8_t bits_ = 0;
    std::string words_;
    std::vector<std::uint32_t> palette_;
    std::vector<std::uint16_t> blocks_;
};

/** A sub-chunk as the network carries it, in format 8 or 9: a storage for each block layer.
 * @see BDS SubChunk, ViaVersion ChunkSection. */
struct SubChunk {
    std::uint8_t version = 9;
    std::uint8_t y_index = 0;
    std::vector<SubChunkStorage> storages;

    static std::expected<SubChunk, std::error_code> read(bp::BinaryReader &in)
    {
        const auto malformed = [] {
            return std::unexpected(std::make_error_code(std::errc::protocol_error));
        };
        SubChunk sub_chunk;
        const auto version = in.read<std::uint8_t>();
        const auto count = in.read<std::uint8_t>();
        if (!version || !count || (*version != 8 && *version != 9)) {
            return malformed();
        }
        sub_chunk.version = *version;
        if (sub_chunk.version == 9) {
            const auto y_index = in.read<std::uint8_t>();
            if (!y_index) {
                return malformed();
            }
            sub_chunk.y_index = *y_index;
        }
        for (std::uint8_t i = 0; i < *count; ++i) {
            auto storage = SubChunkStorage::read(in);
            if (!storage) {
                return std::unexpected(storage.error());
            }
            sub_chunk.storages.push_back(std::move(*storage));
        }
        return sub_chunk;
    }

    void write(bp::BinaryWriter &out) const
    {
        out.write<std::uint8_t>(version);
        out.write<std::uint8_t>(static_cast<std::uint8_t>(storages.size()));
        if (version == 9) {
            out.write<std::uint8_t>(y_index);
        }
        for (const auto &storage : storages) {
            storage.write(out);
        }
    }
};

} // namespace endweave
