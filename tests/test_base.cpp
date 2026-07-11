#include "endweave/connection.h"
#include "endweave/protocol/base.h"
#include "endweave/protocol/packet_ids.h"
#include "endweave/protocol/protocol.h"
#include "recording_log_sink.h"

#include <bedrock/stream.hpp>
#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>

namespace bp = bedrock::protocol;
using endweave::create_base_protocol;
using endweave::Direction;
using endweave::PacketId;
using endweave::TransformResult;
using endweave::UserConnection;

namespace {

std::string be_i32(std::int32_t value)
{
    std::string out;
    bp::BinaryWriter writer{out};
    writer.write<std::int32_t, std::endian::big>(value);
    return out;
}

int id_of(PacketId packet_id)
{
    return static_cast<int>(packet_id);
}

} // namespace

TEST_CASE("detect_client_protocol records the client version and rewrites it to the server's")
{
    RecordingLogSink sink;
    auto base = create_base_protocol(1001);
    UserConnection connection{"a", sink, 1001};

    const std::string tail("\x01\x02\x03", 3);
    const std::string body = be_i32(975) + tail;
    bp::BinaryReader in{body};
    std::string out_buf;
    bp::BinaryWriter out{out_buf};

    auto result = base.transform(Direction::Serverbound, id_of(PacketId::RequestNetworkSettings), connection, in, out);
    REQUIRE(result.has_value());
    CHECK(*result == TransformResult::Translated);
    CHECK(connection.client_protocol() == 975);
    CHECK(out_buf == be_i32(1001) + tail); // rewritten to server version, tail copied verbatim
    CHECK(sink.debugs.size() == 1);
}

TEST_CASE("rewrite_login swaps the client version for the server's")
{
    RecordingLogSink sink;
    auto base = create_base_protocol(1001);
    UserConnection connection{"a", sink, 1001};

    const std::string tail("\xAA\xBB", 2);
    const std::string body = be_i32(975) + tail;
    bp::BinaryReader in{body};
    std::string out_buf;
    bp::BinaryWriter out{out_buf};

    auto result = base.transform(Direction::Serverbound, id_of(PacketId::Login), connection, in, out);
    REQUIRE(result.has_value());
    CHECK(out_buf == be_i32(1001) + tail);
}

TEST_CASE("log_packet_violation is byte-identical and logs a warning")
{
    RecordingLogSink sink;
    auto base = create_base_protocol(1001);
    UserConnection connection{"a", sink, 1001};

    // type=0, severity=0, packetId=0 (zigzag varints), then a length-prefixed "x".
    const std::string body = std::string("\x00\x00\x00", 3) + std::string("\x01x", 2);
    bp::BinaryReader in{body};
    std::string out_buf;
    bp::BinaryWriter out{out_buf};

    auto result = base.transform(Direction::Clientbound, id_of(PacketId::PacketViolationWarning), connection, in, out);
    REQUIRE(result.has_value());
    CHECK(*result == TransformResult::Translated);
    CHECK(out_buf == body); // byte-identical passthrough
    CHECK(sink.warnings.size() == 1);
}
