#include "attribute_layer_goldens.h"
#include "endweave/connection.h"
#include "endweave/pipeline_core.h"
#include "endweave/protocol/manager.h"
#include "endweave/protocol/protocol.h"
#include "endweave/protocol/v1001_to_v975.h"
#include "recording_log_sink.h"

#include <catch2/catch_test_macros.hpp>
#include <vector>

using endweave::ConnectionManager;
using endweave::Direction;
using endweave::PipelineRunResult;
using endweave::Protocol;
using endweave::ProtocolManager;
using endweave::resolve_pipeline;
using endweave::run_pipeline;
using endweave::UserConnection;

namespace {
constexpr int kAttributeLayerSync = 345;
}

TEST_CASE("resolve_pipeline: pre-handshake is base-only and uncached")
{
    RecordingLogSink sink;
    Protocol base{1001, 0, "base", true};
    ProtocolManager manager;
    manager.register_base(base);
    ConnectionManager connections{1001, sink};

    auto &connection = connections.get_or_create("a"); // client_protocol still 0
    auto pipeline = resolve_pipeline(manager, connection);
    REQUIRE(pipeline.size() == 1);
    CHECK(pipeline[0] == &base);
    CHECK_FALSE(connection.protocol_pipeline().has_value()); // not cached until detected
}

TEST_CASE("resolve_pipeline: same version caches base-only, no clientbound")
{
    RecordingLogSink sink;
    Protocol base{1001, 0, "base", true};
    ProtocolManager manager;
    manager.register_base(base);
    ConnectionManager connections{1001, sink};

    auto &connection = connections.get_or_create("a");
    connection.set_client_protocol(1001);
    auto pipeline = resolve_pipeline(manager, connection);
    REQUIRE(pipeline.size() == 1);
    REQUIRE(connection.protocol_pipeline().has_value());
    CHECK_FALSE(connection.clientbound_pipeline().has_value());
}

TEST_CASE("resolve_pipeline: a chain builds base+chain and reversed clientbound")
{
    RecordingLogSink sink;
    Protocol base{1001, 0, "base", true};
    auto down = endweave::v1001_to_v975::create_protocol();
    ProtocolManager manager;
    manager.register_base(base);
    manager.register_protocol(down);
    ConnectionManager connections{1001, sink};

    auto &connection = connections.get_or_create("a");
    connection.set_client_protocol(975);
    auto pipeline = resolve_pipeline(manager, connection);
    REQUIRE(pipeline.size() == 2);
    CHECK(pipeline[0] == &base);
    CHECK(pipeline[1] == &down);

    REQUIRE(connection.clientbound_pipeline().has_value());
    const auto &clientbound = *connection.clientbound_pipeline();
    REQUIRE(clientbound.size() == 2);
    CHECK(clientbound[0] == &base);
    CHECK(clientbound[1] == &down); // single-element chain reversed is itself
}

TEST_CASE("resolve_pipeline: a missing chain warns once and caches base-only")
{
    RecordingLogSink sink;
    Protocol base{1001, 0, "base", true};
    ProtocolManager manager;
    manager.register_base(base);
    ConnectionManager connections{1001, sink};

    auto &connection = connections.get_or_create("a");
    connection.set_client_protocol(500); // no chain to 500
    auto first = resolve_pipeline(manager, connection);
    REQUIRE(first.size() == 1);
    CHECK(sink.warnings.size() == 1);

    auto second = resolve_pipeline(manager, connection); // cached; must not warn again
    CHECK(second.size() == 1);
    CHECK(sink.warnings.size() == 1);
}

TEST_CASE("run_pipeline downgrades a clientbound packet over the chain")
{
    RecordingLogSink sink;
    auto down = endweave::v1001_to_v975::create_protocol();
    std::vector<const Protocol *> pipeline{&down};
    UserConnection connection{"a", sink, 1001};
    connection.set_client_protocol(975);

    auto result = run_pipeline(pipeline, Direction::Clientbound, kAttributeLayerSync, connection, golden_1001);
    CHECK(result.status == PipelineRunResult::Status::Rewritten);
    CHECK(result.payload == golden_975);
}

TEST_CASE("run_pipeline passes unknown ids through unchanged")
{
    RecordingLogSink sink;
    auto down = endweave::v1001_to_v975::create_protocol();
    std::vector<const Protocol *> pipeline{&down};
    UserConnection connection{"a", sink, 1001};

    auto result = run_pipeline(pipeline, Direction::Clientbound, 999, connection, golden_1001);
    CHECK(result.status == PipelineRunResult::Status::Unchanged);
    CHECK(result.payload == golden_1001);
}

TEST_CASE("run_pipeline cancels when a stage cancels")
{
    RecordingLogSink sink;
    Protocol canceller{1001, 975};
    canceller.cancel_clientbound({kAttributeLayerSync});
    std::vector<const Protocol *> pipeline{&canceller};
    UserConnection connection{"a", sink, 1001};

    auto result = run_pipeline(pipeline, Direction::Clientbound, kAttributeLayerSync, connection, golden_1001);
    CHECK(result.status == PipelineRunResult::Status::Cancelled);
}

TEST_CASE("run_pipeline reports the stage that fails to decode")
{
    RecordingLogSink sink;
    auto down = endweave::v1001_to_v975::create_protocol();
    std::vector<const Protocol *> pipeline{&down};
    UserConnection connection{"a", sink, 1001};

    auto result =
        run_pipeline(pipeline, Direction::Clientbound, kAttributeLayerSync, connection, golden_1001.substr(0, 3));
    CHECK(result.status == PipelineRunResult::Status::Failed);
    CHECK(result.failed_stage == &down);
}
