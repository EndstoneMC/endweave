/// Codec unit tests ported from tests/test_codec.py and tests/test_wrapper.py.

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <string>
#include <string_view>

#include "endweave/codec/reader.h"
#include "endweave/codec/types/enums.h"
#include "endweave/codec/types/primitives.h"
#include "endweave/codec/wrapper.h"
#include "endweave/codec/writer.h"

using namespace endweave;

// -- PacketReader tests --

TEST(PacketReader, ReadByte) {
    std::string_view data("\x42\xFF", 2);
    PacketReader reader(data);

    auto v1 = reader.read_byte();
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, 0x42);

    auto v2 = reader.read_byte();
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, 0xFF);

    auto v3 = reader.read_byte();
    EXPECT_FALSE(v3.has_value());
    EXPECT_EQ(v3.error(), ReadError::out_of_bounds);
}

TEST(PacketReader, ReadBool) {
    std::string_view data("\x01\x00", 2);
    PacketReader reader(data);

    auto t = reader.read_bool();
    ASSERT_TRUE(t.has_value());
    EXPECT_TRUE(*t);

    auto f = reader.read_bool();
    ASSERT_TRUE(f.has_value());
    EXPECT_FALSE(*f);
}

TEST(PacketReader, ReadFixedIntegers) {
    // Build a buffer with known LE values
    PacketWriter w;
    w.write_short_le(-1234);
    w.write_ushort_le(0xBEEF);
    w.write_int_le(-100000);
    w.write_uint_le(0xDEADBEEF);
    w.write_int64_le(-9999999999LL);
    auto buf = w.to_string();

    PacketReader r(buf);
    EXPECT_EQ(*r.read_short_le(), -1234);
    EXPECT_EQ(*r.read_ushort_le(), 0xBEEF);
    EXPECT_EQ(*r.read_int_le(), -100000);
    EXPECT_EQ(*r.read_uint_le(), 0xDEADBEEF);
    EXPECT_EQ(*r.read_int64_le(), -9999999999LL);
    EXPECT_FALSE(r.has_remaining());
}

TEST(PacketReader, ReadIntBE) {
    // 0x00000001 in big-endian
    std::string_view data("\x00\x00\x00\x01", 4);
    PacketReader reader(data);
    auto v = reader.read_int_be();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 1);
}

TEST(PacketReader, ReadFloatDouble) {
    PacketWriter w;
    w.write_float_le(3.14f);
    w.write_double_le(2.718281828);
    auto buf = w.to_string();

    PacketReader r(buf);
    auto f = r.read_float_le();
    ASSERT_TRUE(f.has_value());
    EXPECT_NEAR(*f, 3.14f, 0.001f);

    auto d = r.read_double_le();
    ASSERT_TRUE(d.has_value());
    EXPECT_NEAR(*d, 2.718281828, 0.0000001);
}

TEST(PacketReader, ReadUvarint) {
    // 0 -> single byte 0x00
    {
        std::string_view data("\x00", 1);
        PacketReader r(data);
        EXPECT_EQ(*r.read_uvarint(), 0u);
    }
    // 127 -> single byte 0x7F
    {
        std::string_view data("\x7F", 1);
        PacketReader r(data);
        EXPECT_EQ(*r.read_uvarint(), 127u);
    }
    // 128 -> 0x80 0x01
    {
        std::string_view data("\x80\x01", 2);
        PacketReader r(data);
        EXPECT_EQ(*r.read_uvarint(), 128u);
    }
    // 300 -> 0xAC 0x02
    {
        std::string_view data("\xAC\x02", 2);
        PacketReader r(data);
        EXPECT_EQ(*r.read_uvarint(), 300u);
    }
}

TEST(PacketReader, ReadVarint) {
    // Zigzag: 0 -> 0, -1 -> 1, 1 -> 2, -2 -> 3
    PacketWriter w;
    w.write_varint(0);
    w.write_varint(-1);
    w.write_varint(1);
    w.write_varint(-2);
    w.write_varint(2147483647);   // INT32_MAX
    w.write_varint(-2147483648);  // INT32_MIN
    auto buf = w.to_string();

    PacketReader r(buf);
    EXPECT_EQ(*r.read_varint(), 0);
    EXPECT_EQ(*r.read_varint(), -1);
    EXPECT_EQ(*r.read_varint(), 1);
    EXPECT_EQ(*r.read_varint(), -2);
    EXPECT_EQ(*r.read_varint(), 2147483647);
    EXPECT_EQ(*r.read_varint(), -2147483648);
}

TEST(PacketReader, ReadString) {
    PacketWriter w;
    w.write_string("hello");
    w.write_string("");
    w.write_string("world!");
    auto buf = w.to_string();

    PacketReader r(buf);
    EXPECT_EQ(*r.read_string(), "hello");
    EXPECT_EQ(*r.read_string(), "");
    EXPECT_EQ(*r.read_string(), "world!");
}

TEST(PacketReader, ReadRemaining) {
    std::string_view data("abcdef", 6);
    PacketReader r(data);
    (void)r.read_byte(); // skip 'a'
    (void)r.read_byte(); // skip 'b'
    auto rem = r.read_remaining();
    EXPECT_EQ(rem, "cdef");
    EXPECT_FALSE(r.has_remaining());
}

TEST(PacketReader, Skip) {
    std::string_view data("abcdef", 6);
    PacketReader r(data);
    EXPECT_TRUE(r.skip(3).has_value());
    EXPECT_EQ(r.position(), 3u);
    EXPECT_EQ(r.remaining(), 3u);
    EXPECT_FALSE(r.skip(4).has_value()); // out of bounds
}

// -- Template read/write roundtrip tests --

TEST(TypeTags, ByteRoundtrip) {
    PacketWriter w;
    w.write<byte_t>(0x42);
    auto buf = w.to_string();
    PacketReader r(buf);
    EXPECT_EQ(*r.read<byte_t>(), 0x42);
}

TEST(TypeTags, VarIntRoundtrip) {
    PacketWriter w;
    w.write<var_int>(-12345);
    w.write<var_int>(0);
    w.write<var_int>(12345);
    auto buf = w.to_string();
    PacketReader r(buf);
    EXPECT_EQ(*r.read<var_int>(), -12345);
    EXPECT_EQ(*r.read<var_int>(), 0);
    EXPECT_EQ(*r.read<var_int>(), 12345);
}

TEST(TypeTags, UVarIntRoundtrip) {
    PacketWriter w;
    w.write<uvar_int>(0u);
    w.write<uvar_int>(300u);
    w.write<uvar_int>(0xFFFFFFFFu);
    auto buf = w.to_string();
    PacketReader r(buf);
    EXPECT_EQ(*r.read<uvar_int>(), 0u);
    EXPECT_EQ(*r.read<uvar_int>(), 300u);
    EXPECT_EQ(*r.read<uvar_int>(), 0xFFFFFFFFu);
}

TEST(TypeTags, StringRoundtrip) {
    PacketWriter w;
    w.write<string>(std::string("hello world"));
    auto buf = w.to_string();
    PacketReader r(buf);
    EXPECT_EQ(*r.read<string>(), "hello world");
}

TEST(TypeTags, BlockPosRoundtrip) {
    BlockPosValue pos{-100, 64, 200};
    PacketWriter w;
    w.write<block_pos>(pos);
    auto buf = w.to_string();
    PacketReader r(buf);
    auto result = r.read<block_pos>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, pos);
}

TEST(TypeTags, NetworkBlockPosRoundtrip) {
    BlockPosValue pos{-100, 64, 200};
    PacketWriter w;
    w.write<network_block_pos>(pos);
    auto buf = w.to_string();
    PacketReader r(buf);
    auto result = r.read<network_block_pos>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, pos);
}

TEST(TypeTags, Vec3Roundtrip) {
    Vec3Value v{1.5f, -2.5f, 3.0f};
    PacketWriter w;
    w.write<vec3>(v);
    auto buf = w.to_string();
    PacketReader r(buf);
    auto result = r.read<vec3>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, v);
}

TEST(TypeTags, Vec2Roundtrip) {
    Vec2Value v{1.0f, -1.0f};
    PacketWriter w;
    w.write<vec2>(v);
    auto buf = w.to_string();
    PacketReader r(buf);
    auto result = r.read<vec2>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, v);
}

TEST(TypeTags, UuidRoundtrip) {
    std::array<std::uint8_t, 16> id = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    PacketWriter w;
    w.write<uuid>(id);
    auto buf = w.to_string();
    PacketReader r(buf);
    auto result = r.read<uuid>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, id);
}

TEST(TypeTags, VarInt64Roundtrip) {
    PacketWriter w;
    w.write<var_int64>(static_cast<std::int64_t>(-9999999999LL));
    w.write<var_int64>(static_cast<std::int64_t>(0));
    w.write<var_int64>(static_cast<std::int64_t>(9999999999LL));
    auto buf = w.to_string();
    PacketReader r(buf);
    EXPECT_EQ(*r.read<var_int64>(), -9999999999LL);
    EXPECT_EQ(*r.read<var_int64>(), 0LL);
    EXPECT_EQ(*r.read<var_int64>(), 9999999999LL);
}

// -- BlockPos cross-type conversion (the core v924->v944 rewrite) --

TEST(TypeTags, NetworkBlockPosToBlockPos) {
    BlockPosValue pos{10, 64, -30};

    // Write as NetworkBlockPos (v924 format)
    PacketWriter w;
    w.write<network_block_pos>(pos);
    auto buf = w.to_string();

    // Read as NetworkBlockPos, write as BlockPos (v944 format)
    PacketReader r(buf);
    auto val = r.read<network_block_pos>();
    ASSERT_TRUE(val.has_value());

    PacketWriter w2;
    w2.write<block_pos>(*val);
    auto buf2 = w2.to_string();

    // Verify the v944 output
    PacketReader r2(buf2);
    auto result = r2.read<block_pos>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, pos);
}

// -- PacketWrapper tests --

TEST(PacketWrapper, PassthroughVarInt) {
    PacketWriter w;
    w.write_varint(42);
    w.write_varint(-7);
    auto buf = w.to_string();

    PacketWrapper wrapper(buf);
    auto v1 = wrapper.passthrough<var_int>();
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, 42);

    auto v2 = wrapper.passthrough<var_int>();
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, -7);

    auto output = wrapper.to_string();
    EXPECT_EQ(output, buf); // Passthrough should produce identical bytes
}

TEST(PacketWrapper, ReadRemovesField) {
    PacketWriter w;
    w.write_varint(1);  // field to remove
    w.write_varint(2);  // field to keep
    auto buf = w.to_string();

    PacketWrapper wrapper(buf);
    auto removed = wrapper.read<var_int>();
    EXPECT_EQ(*removed, 1);

    auto kept = wrapper.passthrough<var_int>();
    EXPECT_EQ(*kept, 2);

    auto output = wrapper.to_string();

    // Output should only contain varint(2)
    PacketReader verify(output);
    EXPECT_EQ(*verify.read_varint(), 2);
    EXPECT_FALSE(verify.has_remaining());
}

TEST(PacketWrapper, WriteInsertsField) {
    PacketWriter w;
    w.write_varint(2);
    auto buf = w.to_string();

    PacketWrapper wrapper(buf);
    wrapper.write<var_int>(1);  // insert before
    (void)wrapper.passthrough<var_int>();  // copy 2

    auto output = wrapper.to_string();

    PacketReader verify(output);
    EXPECT_EQ(*verify.read_varint(), 1);
    EXPECT_EQ(*verify.read_varint(), 2);
    EXPECT_FALSE(verify.has_remaining());
}

TEST(PacketWrapper, MapNetworkBlockPosToBlockPos) {
    BlockPosValue pos{10, 64, -30};

    PacketWriter w;
    w.write<network_block_pos>(pos);
    w.write_varint(99); // trailing field
    auto buf = w.to_string();

    PacketWrapper wrapper(buf);
    auto val = wrapper.map<network_block_pos, block_pos>();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, pos);

    auto trailing = wrapper.passthrough<var_int>();
    EXPECT_EQ(*trailing, 99);

    auto output = wrapper.to_string();

    // Verify output has block_pos format + trailing
    PacketReader verify(output);
    auto out_pos = verify.read<block_pos>();
    EXPECT_EQ(*out_pos, pos);
    EXPECT_EQ(*verify.read_varint(), 99);
    EXPECT_FALSE(verify.has_remaining());
}

TEST(PacketWrapper, Cancel) {
    PacketWrapper wrapper("");
    EXPECT_FALSE(wrapper.cancelled());
    wrapper.cancel();
    EXPECT_TRUE(wrapper.cancelled());
}

TEST(PacketWrapper, PassthroughAll) {
    std::string_view data("hello world", 11);
    PacketWrapper wrapper(data);
    (void)wrapper.read<byte_t>(); // consume 'h'
    auto remaining = wrapper.passthrough_all();
    EXPECT_EQ(remaining, "ello world");

    auto output = wrapper.to_string();
    EXPECT_EQ(output, "ello world");
}

TEST(PacketWrapper, ToStringAppendsUnread) {
    PacketWriter w;
    w.write_varint(1);
    w.write_varint(2);
    w.write_varint(3);
    auto buf = w.to_string();

    PacketWrapper wrapper(buf);
    (void)wrapper.passthrough<var_int>(); // copy 1
    // leave 2 and 3 unread

    auto output = wrapper.to_string();
    EXPECT_EQ(output, buf); // Should produce identical bytes
}

// -- Writer standalone tests --

TEST(PacketWriter, IntBERoundtrip) {
    PacketWriter w;
    w.write_int_be(0x01020304);
    auto buf = w.to_string();
    EXPECT_EQ(buf.size(), 4u);
    // Big-endian: 01 02 03 04
    EXPECT_EQ(static_cast<std::uint8_t>(buf[0]), 0x01);
    EXPECT_EQ(static_cast<std::uint8_t>(buf[1]), 0x02);
    EXPECT_EQ(static_cast<std::uint8_t>(buf[2]), 0x03);
    EXPECT_EQ(static_cast<std::uint8_t>(buf[3]), 0x04);

    PacketReader r(buf);
    EXPECT_EQ(*r.read_int_be(), 0x01020304);
}

// -- Enum tests --

TEST(Enums, ActorDataIDs) {
    using endweave::ActorDataIDs;
    EXPECT_EQ(static_cast<int>(ActorDataIDs::HEARTBEAT_SOUND_EVENT), 126);
    EXPECT_EQ(static_cast<int>(ActorDataIDs::AIM_ASSIST_PRIORITY_PRESET_ID), 136);
    EXPECT_EQ(static_cast<int>(ActorDataIDs::Count), 139);
}

TEST(Enums, LevelSoundEvent) {
    using endweave::LevelSoundEvent;
    EXPECT_EQ(static_cast<unsigned int>(LevelSoundEvent::ItemUseOn), 0u);
    EXPECT_EQ(static_cast<unsigned int>(LevelSoundEvent::Lunge1), 566u);
    EXPECT_EQ(static_cast<unsigned int>(LevelSoundEvent::SaddleInWater), 578u);
    EXPECT_EQ(static_cast<unsigned int>(LevelSoundEvent::PauseGrowth), 597u);
    EXPECT_EQ(static_cast<unsigned int>(LevelSoundEvent::Undefined), 599u);
}

TEST(Enums, SoundBoundaries) {
    EXPECT_EQ(endweave::sound_boundaries::v860_start, 566u);
    EXPECT_EQ(endweave::sound_boundaries::v898_start, 578u);
    EXPECT_EQ(endweave::sound_boundaries::v944_start, 597u);
    EXPECT_EQ(endweave::sound_boundaries::undefined, 599u);
}

TEST(Enums, VariousEnums) {
    using namespace endweave;
    EXPECT_EQ(static_cast<int>(DataItemType::Int), 2);
    EXPECT_EQ(static_cast<int>(DataItemType::Int64), 7);
    EXPECT_EQ(static_cast<int>(InteractAction::StopRiding), 3);
    EXPECT_EQ(static_cast<int>(TextPacketType::Chat), 1);
    EXPECT_EQ(static_cast<int>(GameRuleType::Bool), 1);
    EXPECT_EQ(static_cast<int>(ComplexInventoryTransactionType::ItemUseTransaction), 2);
    EXPECT_EQ(static_cast<int>(CommandOriginType::Test), 4);
    EXPECT_EQ(static_cast<int>(CommandPermissionLevel::Any), 0);
}
