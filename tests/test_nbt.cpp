/// NBT codec tests ported from the Python test suite.

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "endweave/codec/reader.h"
#include "endweave/codec/types/nbt.h"
#include "endweave/codec/wrapper.h"
#include "endweave/codec/writer.h"

using namespace endweave;
using namespace endweave::nbt;

// -- Helpers --

/// Write NBT, read it back, verify roundtrip.
static std::unique_ptr<CompoundTag> roundtrip(const CompoundTag& tag, bool named = true) {
    PacketWriter w;
    write_nbt(w, &tag, named ? "" : nullptr);
    auto buf = w.to_string();
    PacketReader r(buf);
    auto result = read_nbt(r, named);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(r.has_remaining());
    return std::move(*result);
}

// -- Basic tag type roundtrips --

TEST(Nbt, EmptyCompound) {
    CompoundTag tag;
    auto result = roundtrip(tag);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->value.empty());
}

TEST(Nbt, ByteTag) {
    CompoundTag tag;
    tag.set("b", std::make_unique<ByteTag>(42));
    auto result = roundtrip(tag);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->contains("b"));
    auto& b = static_cast<const ByteTag&>((*result)["b"]);
    EXPECT_EQ(b.value, 42);
}

TEST(Nbt, ShortTag) {
    CompoundTag tag;
    tag.set("s", std::make_unique<ShortTag>(-1234));
    auto result = roundtrip(tag);
    auto& s = static_cast<const ShortTag&>((*result)["s"]);
    EXPECT_EQ(s.value, -1234);
}

TEST(Nbt, IntTag) {
    CompoundTag tag;
    tag.set("i", std::make_unique<IntTag>(-100000));
    auto result = roundtrip(tag);
    auto& i = static_cast<const IntTag&>((*result)["i"]);
    EXPECT_EQ(i.value, -100000);
}

TEST(Nbt, LongTag) {
    CompoundTag tag;
    tag.set("l", std::make_unique<LongTag>(-9999999999LL));
    auto result = roundtrip(tag);
    auto& l = static_cast<const LongTag&>((*result)["l"]);
    EXPECT_EQ(l.value, -9999999999LL);
}

TEST(Nbt, FloatTag) {
    CompoundTag tag;
    tag.set("f", std::make_unique<FloatTag>(3.14f));
    auto result = roundtrip(tag);
    auto& f = static_cast<const FloatTag&>((*result)["f"]);
    EXPECT_NEAR(f.value, 3.14f, 0.001f);
}

TEST(Nbt, DoubleTag) {
    CompoundTag tag;
    tag.set("d", std::make_unique<DoubleTag>(2.718281828));
    auto result = roundtrip(tag);
    auto& d = static_cast<const DoubleTag&>((*result)["d"]);
    EXPECT_NEAR(d.value, 2.718281828, 0.0000001);
}

TEST(Nbt, StringTag) {
    CompoundTag tag;
    tag.set("s", std::make_unique<StringTag>("hello world"));
    auto result = roundtrip(tag);
    auto& s = static_cast<const StringTag&>((*result)["s"]);
    EXPECT_EQ(s.value, "hello world");
}

TEST(Nbt, ByteArrayTag) {
    CompoundTag tag;
    tag.set("arr", std::make_unique<ByteArrayTag>(std::vector<std::uint8_t>{1, 2, 3, 4, 5}));
    auto result = roundtrip(tag);
    auto& arr = static_cast<const ByteArrayTag&>((*result)["arr"]);
    EXPECT_EQ(arr.value, (std::vector<std::uint8_t>{1, 2, 3, 4, 5}));
}

TEST(Nbt, IntArrayTag) {
    CompoundTag tag;
    tag.set("arr", std::make_unique<IntArrayTag>(std::vector<std::int32_t>{-1, 0, 1, 100000}));
    auto result = roundtrip(tag);
    auto& arr = static_cast<const IntArrayTag&>((*result)["arr"]);
    EXPECT_EQ(arr.value, (std::vector<std::int32_t>{-1, 0, 1, 100000}));
}

TEST(Nbt, LongArrayTag) {
    CompoundTag tag;
    tag.set("arr", std::make_unique<LongArrayTag>(std::vector<std::int64_t>{-1LL, 0LL, 9999999999LL}));
    auto result = roundtrip(tag);
    auto& arr = static_cast<const LongArrayTag&>((*result)["arr"]);
    EXPECT_EQ(arr.value, (std::vector<std::int64_t>{-1LL, 0LL, 9999999999LL}));
}

// -- Nested structures --

TEST(Nbt, ListOfInts) {
    CompoundTag tag;
    std::vector<std::unique_ptr<Tag>> items;
    items.push_back(std::make_unique<IntTag>(10));
    items.push_back(std::make_unique<IntTag>(20));
    items.push_back(std::make_unique<IntTag>(30));
    tag.set("list", std::make_unique<ListTag>(TAG_INT, std::move(items)));
    auto result = roundtrip(tag);
    auto& list = static_cast<const ListTag&>((*result)["list"]);
    EXPECT_EQ(list.element_type, TAG_INT);
    EXPECT_EQ(list.size(), 3u);
    EXPECT_EQ(static_cast<const IntTag&>(list[0]).value, 10);
    EXPECT_EQ(static_cast<const IntTag&>(list[1]).value, 20);
    EXPECT_EQ(static_cast<const IntTag&>(list[2]).value, 30);
}

TEST(Nbt, EmptyList) {
    CompoundTag tag;
    tag.set("empty", std::make_unique<ListTag>(TAG_END, std::vector<std::unique_ptr<Tag>>{}));
    auto result = roundtrip(tag);
    auto& list = static_cast<const ListTag&>((*result)["empty"]);
    EXPECT_EQ(list.size(), 0u);
}

TEST(Nbt, NestedCompound) {
    CompoundTag inner;
    inner.set("x", std::make_unique<IntTag>(42));
    inner.set("y", std::make_unique<StringTag>("nested"));

    CompoundTag outer;
    auto inner_ptr = std::make_unique<CompoundTag>();
    inner_ptr->set("x", std::make_unique<IntTag>(42));
    inner_ptr->set("y", std::make_unique<StringTag>("nested"));
    outer.set("inner", std::move(inner_ptr));

    auto result = roundtrip(outer);
    ASSERT_TRUE(result->contains("inner"));
    auto& r_inner = static_cast<const CompoundTag&>((*result)["inner"]);
    EXPECT_EQ(static_cast<const IntTag&>(r_inner["x"]).value, 42);
    EXPECT_EQ(static_cast<const StringTag&>(r_inner["y"]).value, "nested");
}

TEST(Nbt, ListOfCompounds) {
    std::vector<std::unique_ptr<Tag>> items;

    auto c1 = std::make_unique<CompoundTag>();
    c1->set("name", std::make_unique<StringTag>("alpha"));
    c1->set("id", std::make_unique<IntTag>(1));
    items.push_back(std::move(c1));

    auto c2 = std::make_unique<CompoundTag>();
    c2->set("name", std::make_unique<StringTag>("beta"));
    c2->set("id", std::make_unique<IntTag>(2));
    items.push_back(std::move(c2));

    CompoundTag tag;
    tag.set("entries", std::make_unique<ListTag>(TAG_COMPOUND, std::move(items)));

    auto result = roundtrip(tag);
    auto& list = static_cast<const ListTag&>((*result)["entries"]);
    EXPECT_EQ(list.size(), 2u);
    auto& e1 = static_cast<const CompoundTag&>(list[0]);
    EXPECT_EQ(static_cast<const StringTag&>(e1["name"]).value, "alpha");
    auto& e2 = static_cast<const CompoundTag&>(list[1]);
    EXPECT_EQ(static_cast<const IntTag&>(e2["id"]).value, 2);
}

// -- Absent NBT --

TEST(Nbt, AbsentNbt) {
    PacketWriter w;
    write_nbt(w, nullptr);
    auto buf = w.to_string();
    EXPECT_EQ(buf.size(), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(buf[0]), TAG_END);

    PacketReader r(buf);
    auto result = read_nbt(r);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, nullptr);
}

// -- Named vs nameless format --

TEST(Nbt, NamedFormat) {
    CompoundTag tag;
    tag.set("val", std::make_unique<IntTag>(7));
    auto result = roundtrip(tag, true);
    EXPECT_EQ(static_cast<const IntTag&>((*result)["val"]).value, 7);
}

TEST(Nbt, NamelessFormat) {
    CompoundTag tag;
    tag.set("val", std::make_unique<IntTag>(7));
    auto result = roundtrip(tag, false);
    EXPECT_EQ(static_cast<const IntTag&>((*result)["val"]).value, 7);
}

// -- PacketWrapper type tag integration --

TEST(Nbt, WrapperPassthroughNamed) {
    CompoundTag tag;
    tag.set("x", std::make_unique<IntTag>(99));

    PacketWriter w;
    w.write<named_compound_tag>(std::make_unique<CompoundTag>(std::move(tag)));
    auto buf = w.to_string();

    PacketWrapper wrapper(buf);
    auto val = wrapper.passthrough<named_compound_tag>();
    ASSERT_TRUE(val.has_value());
    ASSERT_NE(*val, nullptr);
    EXPECT_EQ(static_cast<const IntTag&>((**val)["x"]).value, 99);

    auto output = wrapper.to_string();
    EXPECT_EQ(output, buf);
}

TEST(Nbt, WrapperPassthroughNameless) {
    CompoundTag tag;
    tag.set("y", std::make_unique<StringTag>("test"));

    PacketWriter w;
    w.write<compound_tag>(std::make_unique<CompoundTag>(std::move(tag)));
    auto buf = w.to_string();

    PacketWrapper wrapper(buf);
    auto val = wrapper.passthrough<compound_tag>();
    ASSERT_TRUE(val.has_value());
    ASSERT_NE(*val, nullptr);
    EXPECT_EQ(static_cast<const StringTag&>((**val)["y"]).value, "test");

    auto output = wrapper.to_string();
    EXPECT_EQ(output, buf);
}

// -- Clone --

TEST(Nbt, CloneCompound) {
    CompoundTag tag;
    tag.set("a", std::make_unique<IntTag>(1));
    tag.set("b", std::make_unique<StringTag>("two"));

    auto cloned = tag.clone();
    auto& c = static_cast<CompoundTag&>(*cloned);
    EXPECT_EQ(static_cast<const IntTag&>(c["a"]).value, 1);
    EXPECT_EQ(static_cast<const StringTag&>(c["b"]).value, "two");

    // Mutating the clone should not affect the original
    c.set("a", std::make_unique<IntTag>(999));
    EXPECT_EQ(static_cast<const IntTag&>(tag["a"]).value, 1);
}

// -- CompoundTag operations --

TEST(Nbt, CompoundErase) {
    CompoundTag tag;
    tag.set("a", std::make_unique<IntTag>(1));
    tag.set("b", std::make_unique<IntTag>(2));
    EXPECT_TRUE(tag.contains("a"));
    tag.erase("a");
    EXPECT_FALSE(tag.contains("a"));
    EXPECT_TRUE(tag.contains("b"));
}

TEST(Nbt, CompoundGet) {
    CompoundTag tag;
    tag.set("a", std::make_unique<IntTag>(42));
    EXPECT_NE(tag.get("a"), nullptr);
    EXPECT_EQ(tag.get("missing"), nullptr);
}
