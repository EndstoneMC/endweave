#include "attribute_layer_goldens.h"
#include "endweave/connection.h"
#include "endweave/log_sink.h"
#include "endweave/protocol/attribute_layer_sync.h"
#include "endweave/protocol/protocol.h"
#include "endweave/protocol/v1001_to_v975.h"
#include "endweave/protocol/v975_to_v1001.h"

#include <bedrock/stream.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace bp = bedrock::protocol;
using endweave::Direction;
using endweave::Protocol;
using endweave::TransformResult;
using endweave::UserConnection;

namespace {

constexpr int kAttributeLayerSync = 345;
constexpr int kUpdateSoundData = 348;

// The version protocols are connection-stateless, so any connection works.
endweave::LogSink g_sink;

UserConnection make_connection()
{
    return UserConnection{"test", g_sink, 1001};
}

std::string transform_out(const Protocol &protocol, Direction direction, int packet_id, const std::string &in_bytes,
                          TransformResult &result)
{
    auto connection = make_connection();
    bp::BinaryReader in{in_bytes};
    std::string buffer;
    bp::BinaryWriter out{buffer};
    auto outcome = protocol.transform(direction, packet_id, connection, in, out);
    REQUIRE(outcome.has_value());
    result = *outcome;
    return buffer;
}

} // namespace

TEST_CASE("v975_to_v1001 upgrades a clientbound attribute-layer-sync body")
{
    const auto protocol = endweave::v975_to_v1001::create_protocol();
    CHECK(protocol.server_protocol() == 975);
    CHECK(protocol.client_protocol() == 1001);

    auto connection = make_connection();
    bp::BinaryReader in{golden_975};
    std::string buffer;
    bp::BinaryWriter out{buffer};
    auto result = protocol.transform(Direction::Clientbound, kAttributeLayerSync, connection, in, out);

    REQUIRE(result.has_value());
    CHECK(*result == TransformResult::Translated);
    CHECK(buffer == golden_1001);
    CHECK(in.getUnreadLength() == 0);
}

TEST_CASE("v1001_to_v975 downgrades a clientbound attribute-layer-sync body")
{
    const auto protocol = endweave::v1001_to_v975::create_protocol();
    CHECK(protocol.server_protocol() == 1001);
    CHECK(protocol.client_protocol() == 975);

    TransformResult result{};
    auto buffer = transform_out(protocol, Direction::Clientbound, kAttributeLayerSync, golden_1001, result);

    CHECK(result == TransformResult::Translated);
    CHECK(buffer == golden_975);
}

TEST_CASE("unregistered ids and directions pass through")
{
    const auto protocol = endweave::v975_to_v1001::create_protocol();

    CHECK(protocol.has_handler_or_cancel(Direction::Clientbound, kAttributeLayerSync));
    CHECK_FALSE(protocol.has_handler_or_cancel(Direction::Serverbound, kAttributeLayerSync));
    CHECK_FALSE(protocol.has_handler_or_cancel(Direction::Clientbound, 999));

    TransformResult result{};
    auto buffer = transform_out(protocol, Direction::Clientbound, 999, golden_975, result);
    CHECK(result == TransformResult::Passthrough);
    CHECK(buffer.empty());
}

TEST_CASE("a cancelled id drops the packet")
{
    Protocol protocol{1001, 975};
    protocol.cancel_clientbound({kAttributeLayerSync});

    CHECK(protocol.has_handler_or_cancel(Direction::Clientbound, kAttributeLayerSync));

    TransformResult result{};
    auto buffer = transform_out(protocol, Direction::Clientbound, kAttributeLayerSync, golden_1001, result);
    CHECK(result == TransformResult::Cancelled);
}

TEST_CASE("v1001_to_v975 cancels the 1001-only update-sound-data packet")
{
    const auto protocol = endweave::v1001_to_v975::create_protocol();
    CHECK(protocol.has_handler_or_cancel(Direction::Clientbound, kUpdateSoundData));

    TransformResult result{};
    transform_out(protocol, Direction::Clientbound, kUpdateSoundData, /*body=*/"", result);
    CHECK(result == TransformResult::Cancelled);
}

TEST_CASE("v975_to_v1001 leaves update-sound-data untouched")
{
    const auto protocol = endweave::v975_to_v1001::create_protocol();
    CHECK_FALSE(protocol.has_handler_or_cancel(Direction::Clientbound, kUpdateSoundData));
}

TEST_CASE("a truncated body surfaces the codec error")
{
    const auto protocol = endweave::v975_to_v1001::create_protocol();

    auto connection = make_connection();
    const std::string truncated = golden_975.substr(0, 3); // payload type + a string length with no bytes behind it
    bp::BinaryReader in{truncated};                         // named so the reader's view does not dangle
    std::string buffer;
    bp::BinaryWriter out{buffer};
    auto result = protocol.transform(Direction::Clientbound, kAttributeLayerSync, connection, in, out);

    CHECK_FALSE(result.has_value());
}
