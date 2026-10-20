#pragma once

#include <bedrock/protocol/nbt.hpp>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace bp = bedrock::protocol;

namespace endweave {

/** Reads SNBT: compounds, lists, bytes (`1b`), ints and strings, which are the tags block states use.
 * @see Mojang TagParser. */
class TagParser {
public:
    static std::optional<bp::Tag> parseTag(std::string_view text)
    {
        TagParser parser{text};
        auto tag = parser.readValue();
        parser.skipWhitespace();
        if (!tag || parser.pos_ != text.size()) {
            return std::nullopt;
        }
        return tag;
    }

private:
    explicit TagParser(std::string_view text) : text_(text) {}

    void skipWhitespace()
    {
        while (pos_ < text_.size() && (text_[pos_] == ' ' || text_[pos_] == '\n')) {
            ++pos_;
        }
    }

    bool consume(char c)
    {
        skipWhitespace();
        if (pos_ < text_.size() && text_[pos_] == c) {
            ++pos_;
            return true;
        }
        return false;
    }

    static bool isAllowedInUnquotedString(char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '-' ||
               c == '.' || c == '+';
    }

    std::optional<std::string> readString()
    {
        skipWhitespace();
        if (pos_ == text_.size()) {
            return std::nullopt;
        }
        if (const char quote = text_[pos_]; quote == '\'' || quote == '"') {
            std::string result;
            for (++pos_; pos_ < text_.size() && text_[pos_] != quote; ++pos_) {
                if (text_[pos_] == '\\' && pos_ + 1 < text_.size()) {
                    ++pos_;
                }
                result += text_[pos_];
            }
            if (pos_ == text_.size()) {
                return std::nullopt;
            }
            ++pos_;
            return result;
        }
        const auto start = pos_;
        while (pos_ < text_.size() && isAllowedInUnquotedString(text_[pos_])) {
            ++pos_;
        }
        if (pos_ == start) {
            return std::nullopt;
        }
        return std::string(text_.substr(start, pos_ - start));
    }

    std::optional<bp::Tag> readValue()
    {
        if (consume('{')) {
            return readCompound();
        }
        if (consume('[')) {
            return readList();
        }
        const bool quoted = pos_ < text_.size() && (text_[pos_] == '\'' || text_[pos_] == '"');
        auto text = readString();
        if (!text) {
            return std::nullopt;
        }
        if (quoted) {
            return bp::Tag{bp::StringTag{std::move(*text)}};
        }
        std::string_view number = *text;
        const bool byte = number.ends_with('b') || number.ends_with('B');
        if (byte) {
            number.remove_suffix(1);
        }
        std::int32_t value = 0;
        const auto [end, error] = std::from_chars(number.data(), number.data() + number.size(), value);
        if (error != std::errc{} || end != number.data() + number.size()) {
            return bp::Tag{bp::StringTag{std::move(*text)}};
        }
        if (byte) {
            return bp::Tag{bp::ByteTag{static_cast<std::int8_t>(value)}};
        }
        return bp::Tag{bp::IntTag{value}};
    }

    std::optional<bp::Tag> readCompound()
    {
        bp::CompoundTag compound;
        if (consume('}')) {
            return bp::Tag{std::move(compound)};
        }
        do {
            auto key = readString();
            if (!key || !consume(':')) {
                return std::nullopt;
            }
            auto value = readValue();
            if (!value) {
                return std::nullopt;
            }
            compound.insert_or_assign(std::move(*key), std::move(*value));
        } while (consume(','));
        if (!consume('}')) {
            return std::nullopt;
        }
        return bp::Tag{std::move(compound)};
    }

    std::optional<bp::Tag> readList()
    {
        bp::ListTag list;
        if (consume(']')) {
            return bp::Tag{std::move(list)};
        }
        do {
            auto value = readValue();
            if (!value) {
                return std::nullopt;
            }
            list.push_back(std::move(*value));
        } while (consume(','));
        if (!consume(']')) {
            return std::nullopt;
        }
        return bp::Tag{std::move(list)};
    }

    std::string_view text_;
    std::size_t pos_ = 0;
};

} // namespace endweave
