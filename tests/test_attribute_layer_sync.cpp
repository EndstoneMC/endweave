#include <cstdint>
#include <string>

#include <bedrock/protocol.hpp>
#include <catch2/catch_test_macros.hpp>

#include "endweave/protocol/attribute_layer_sync.h"

namespace bp = bedrock::protocol;

namespace {

template <class T>
std::string encode(const T &value)
{
    std::string buffer;
    bp::BinaryWriter writer{buffer};
    bp::Serializer<T>::serialize(writer, value);
    return buffer;
}

template <class T>
T decode(const std::string &data)
{
    bp::BinaryReader reader{data};
    auto out = bp::Serializer<T>::deserialize(reader);
    REQUIRE(out.has_value());
    REQUIRE(reader.getUnreadLength() == 0);
    return *out;
}

std::string bytes(std::initializer_list<int> raw)
{
    std::string out;
    for (int b : raw) {
        out.push_back(static_cast<char>(b));
    }
    return out;
}

// A case-2 (UpdateEnvironmentAttributes) packet carrying one bool attribute --
// the same body the bedrock-protocol goldens use.
const std::string golden_975 = bytes({
    0x02,                                              // payload type = UpdateEnvironmentAttributes
    0x03, 0x77, 0x65, 0x74,                            // layer name "wet"
    0x00,                                              // dimension = 0
    0x01,                                              // attribute count = 1
    0x04, 0x74, 0x65, 0x6d, 0x70,                      // attribute name "temp"
    0x00,                                              // from_attribute absent
    0x00,                                              //   attribute type = bool
    0x01,                                              //   value = true
    0x08, 0x4f, 0x56, 0x45, 0x52, 0x52, 0x49, 0x44, 0x45,  //   operation "OVERRIDE"
    0x00,                                              // to_attribute absent
    0x00, 0x00, 0x00, 0x00,                            // current_transition_ticks = 0
    0x00, 0x00, 0x00, 0x00,                            // total_transition_ticks = 0
    0x06, 0x6c, 0x69, 0x6e, 0x65, 0x61, 0x72,          // easing "linear"
});

// v1001 = v975 plus the two 976-step EnvironmentAttributeData fields, at neutral
// defaults -- so the upgrade polyfill reproduces it exactly and the downgrade
// drops back to golden_975.
const std::string golden_1001 = golden_975 + bytes({
    0x00, 0x00, 0x00, 0x00,  // local_transition_ticks = 0
    0x00,                    // noise_transition = false
});

using PacketV975 = endweave::AttributeLayerSyncV975;
using PacketV1001 = endweave::AttributeLayerSyncV1001;

bp::v1001::EnvironmentAttributeData &first_env(PacketV1001 &p)
{
    return std::get<2>(p.data).attributes.at(0);
}

}  // namespace

TEST_CASE("packet id is 345 at both versions")
{
    STATIC_REQUIRE(PacketV975::Id == 345);
    STATIC_REQUIRE(PacketV1001::Id == 345);
}

TEST_CASE("goldens self-round-trip through the codec")
{
    auto p975 = decode<PacketV975>(golden_975);
    REQUIRE(p975.data.index() == 2);
    REQUIRE(encode(p975) == golden_975);

    auto p1001 = decode<PacketV1001>(golden_1001);
    REQUIRE(p1001.data.index() == 2);
    REQUIRE(encode(p1001) == golden_1001);
}

TEST_CASE("upgrade 975 -> 1001 polyfills the new fields")
{
    auto upgraded = endweave::upgrade(decode<PacketV975>(golden_975));
    CHECK(first_env(upgraded).local_transition_ticks == 0);
    CHECK(first_env(upgraded).noise_transition == false);
    REQUIRE(encode(upgraded) == golden_1001);
}

TEST_CASE("downgrade 1001 -> 975 drops the new fields")
{
    auto downgraded = endweave::downgrade(decode<PacketV1001>(golden_1001));
    REQUIRE(encode(downgraded) == golden_975);
}

TEST_CASE("upgrade then downgrade is lossless for the shared fields")
{
    auto back = endweave::downgrade(endweave::upgrade(decode<PacketV975>(golden_975)));
    REQUIRE(encode(back) == golden_975);
}

TEST_CASE("downgrade is lossy: non-default new fields are discarded")
{
    auto p1001 = decode<PacketV1001>(golden_1001);
    first_env(p1001).local_transition_ticks = 42;
    first_env(p1001).noise_transition = true;
    // The 975 wire has no room for them, so downgrade must drop them.
    REQUIRE(encode(endweave::downgrade(p1001)) == golden_975);
}
