#pragma once

/// Bedrock network NBT tag hierarchy and codec.
///
/// Bedrock network NBT differs from Java Edition:
/// - String lengths use uvarint (not big-endian short)
/// - Int values use zigzag varint (not fixed 4 bytes big-endian)
/// - Int64 values use zigzag varint64 (not fixed 8 bytes big-endian)
/// - Shorts, floats, and doubles are little-endian (not big-endian)
///
/// See Also:
///     com.viaversion.viaversion.api.type.types.misc.NamedCompoundTagType

#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "endweave/codec/reader.h"
#include "endweave/codec/writer.h"

namespace endweave::nbt {

// Tag type IDs
inline constexpr int TAG_END = 0;
inline constexpr int TAG_BYTE = 1;
inline constexpr int TAG_SHORT = 2;
inline constexpr int TAG_INT = 3;
inline constexpr int TAG_LONG = 4;
inline constexpr int TAG_FLOAT = 5;
inline constexpr int TAG_DOUBLE = 6;
inline constexpr int TAG_BYTE_ARRAY = 7;
inline constexpr int TAG_STRING = 8;
inline constexpr int TAG_LIST = 9;
inline constexpr int TAG_COMPOUND = 10;
inline constexpr int TAG_INT_ARRAY = 11;
inline constexpr int TAG_LONG_ARRAY = 12;

inline constexpr int MAX_DEPTH = 512;

class Tag {
public:
    virtual ~Tag() = default;
    [[nodiscard]] virtual int tag_id() const = 0;
    [[nodiscard]] virtual std::unique_ptr<Tag> clone() const = 0;
};

class ByteTag final : public Tag {
public:
    std::int8_t value = 0;
    explicit ByteTag(std::int8_t v = 0) : value(v) {}
    [[nodiscard]] int tag_id() const override { return TAG_BYTE; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<ByteTag>(value); }
};

class ShortTag final : public Tag {
public:
    std::int16_t value = 0;
    explicit ShortTag(std::int16_t v = 0) : value(v) {}
    [[nodiscard]] int tag_id() const override { return TAG_SHORT; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<ShortTag>(value); }
};

class IntTag final : public Tag {
public:
    std::int32_t value = 0;
    explicit IntTag(std::int32_t v = 0) : value(v) {}
    [[nodiscard]] int tag_id() const override { return TAG_INT; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<IntTag>(value); }
};

class LongTag final : public Tag {
public:
    std::int64_t value = 0;
    explicit LongTag(std::int64_t v = 0) : value(v) {}
    [[nodiscard]] int tag_id() const override { return TAG_LONG; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<LongTag>(value); }
};

class FloatTag final : public Tag {
public:
    float value = 0.0f;
    explicit FloatTag(float v = 0.0f) : value(v) {}
    [[nodiscard]] int tag_id() const override { return TAG_FLOAT; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<FloatTag>(value); }
};

class DoubleTag final : public Tag {
public:
    double value = 0.0;
    explicit DoubleTag(double v = 0.0) : value(v) {}
    [[nodiscard]] int tag_id() const override { return TAG_DOUBLE; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<DoubleTag>(value); }
};

class ByteArrayTag final : public Tag {
public:
    std::vector<std::uint8_t> value;
    explicit ByteArrayTag(std::vector<std::uint8_t> v = {}) : value(std::move(v)) {}
    [[nodiscard]] int tag_id() const override { return TAG_BYTE_ARRAY; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<ByteArrayTag>(value); }
};

class StringTag final : public Tag {
public:
    std::string value;
    explicit StringTag(std::string v = "") : value(std::move(v)) {}
    [[nodiscard]] int tag_id() const override { return TAG_STRING; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<StringTag>(value); }
};

class ListTag final : public Tag {
public:
    int element_type = TAG_END;
    std::vector<std::unique_ptr<Tag>> tags;

    ListTag() = default;
    ListTag(int elem_type, std::vector<std::unique_ptr<Tag>> t)
        : element_type(elem_type), tags(std::move(t)) {}

    [[nodiscard]] int tag_id() const override { return TAG_LIST; }

    [[nodiscard]] std::unique_ptr<Tag> clone() const override {
        auto copy = std::make_unique<ListTag>();
        copy->element_type = element_type;
        copy->tags.reserve(tags.size());
        for (auto& t : tags) copy->tags.push_back(t->clone());
        return copy;
    }

    [[nodiscard]] Tag& operator[](std::size_t i) { return *tags[i]; }
    [[nodiscard]] const Tag& operator[](std::size_t i) const { return *tags[i]; }
    [[nodiscard]] std::size_t size() const { return tags.size(); }
};

class CompoundTag final : public Tag {
public:
    // std::map preserves insertion order for iteration (sorted by key).
    // Python dict preserves insertion order, but BDS uses sorted keys in practice.
    std::map<std::string, std::unique_ptr<Tag>> value;

    CompoundTag() = default;

    [[nodiscard]] int tag_id() const override { return TAG_COMPOUND; }

    [[nodiscard]] std::unique_ptr<Tag> clone() const override {
        auto copy = std::make_unique<CompoundTag>();
        for (auto& [k, v] : value) copy->value[k] = v->clone();
        return copy;
    }

    Tag& operator[](const std::string& key) { return *value.at(key); }
    const Tag& operator[](const std::string& key) const { return *value.at(key); }

    void set(const std::string& key, std::unique_ptr<Tag> tag) {
        value[key] = std::move(tag);
    }

    void erase(const std::string& key) { value.erase(key); }

    [[nodiscard]] bool contains(const std::string& key) const {
        return value.contains(key);
    }

    [[nodiscard]] Tag* get(const std::string& key) {
        auto it = value.find(key);
        return it != value.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const Tag* get(const std::string& key) const {
        auto it = value.find(key);
        return it != value.end() ? it->second.get() : nullptr;
    }
};

class IntArrayTag final : public Tag {
public:
    std::vector<std::int32_t> value;
    explicit IntArrayTag(std::vector<std::int32_t> v = {}) : value(std::move(v)) {}
    [[nodiscard]] int tag_id() const override { return TAG_INT_ARRAY; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<IntArrayTag>(value); }
};

class LongArrayTag final : public Tag {
public:
    std::vector<std::int64_t> value;
    explicit LongArrayTag(std::vector<std::int64_t> v = {}) : value(std::move(v)) {}
    [[nodiscard]] int tag_id() const override { return TAG_LONG_ARRAY; }
    [[nodiscard]] std::unique_ptr<Tag> clone() const override { return std::make_unique<LongArrayTag>(value); }
};

// -- Read/Write functions --

/// Read a tag value (without header) given its type ID.
std::expected<std::unique_ptr<Tag>, ReadError> read_value(PacketReader& reader, int tag_type, int depth);

/// Read a network NBT root compound tag.
/// Returns nullptr for absent NBT (TAG_END root).
/// If read_name is true, reads and discards the root name string (Bedrock named format).
std::expected<std::unique_ptr<CompoundTag>, ReadError> read_nbt(PacketReader& reader, bool read_name = true);

/// Write a tag value (without header).
void write_value(PacketWriter& writer, const Tag& tag);

/// Write a network NBT root compound tag.
/// nullptr is written as a single End byte (0).
/// If name is non-null, writes the root name string (Bedrock named format).
void write_nbt(PacketWriter& writer, const CompoundTag* tag, const char* name = "");

} // namespace endweave::nbt

// -- Type tags for PacketWrapper --

namespace endweave {

/// NBT CompoundTag with root name prefix (Bedrock network format).
struct named_compound_tag {
    using value_type = std::unique_ptr<nbt::CompoundTag>;
};

/// NBT CompoundTag without root name prefix (Java 1.20.2+ format).
struct compound_tag {
    using value_type = std::unique_ptr<nbt::CompoundTag>;
};

// Reader specializations
template <>
auto PacketReader::read<named_compound_tag>() -> std::expected<std::unique_ptr<nbt::CompoundTag>, ReadError>;

template <>
auto PacketReader::read<compound_tag>() -> std::expected<std::unique_ptr<nbt::CompoundTag>, ReadError>;

// Writer specializations
template <>
void PacketWriter::write<named_compound_tag>(const std::unique_ptr<nbt::CompoundTag>& val);

template <>
void PacketWriter::write<compound_tag>(const std::unique_ptr<nbt::CompoundTag>& val);

} // namespace endweave
