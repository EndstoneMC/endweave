#include "endweave/connection.h"
#include "endweave/log_sink.h"

#include <catch2/catch_test_macros.hpp>

using endweave::ConnectionManager;
using endweave::LogSink;
using endweave::UserConnection;

TEST_CASE("get_or_create is idempotent; get finds and misses")
{
    LogSink sink;
    ConnectionManager connections{1001, sink};
    auto &a = connections.get_or_create("host:1");
    a.set_client_protocol(975);
    auto &b = connections.get_or_create("host:1");
    CHECK(&a == &b);
    CHECK(b.client_protocol() == 975);
    CHECK(connections.get("host:1") == &a);
    CHECK(connections.get("absent") == nullptr);
}

TEST_CASE("needs_translation truth table")
{
    LogSink sink;
    UserConnection connection{"a", sink, 1001};
    CHECK_FALSE(connection.needs_translation()); // client undetected (0)
    connection.set_client_protocol(1001);
    CHECK_FALSE(connection.needs_translation()); // same version
    connection.set_client_protocol(975);
    CHECK(connection.needs_translation()); // different version
}

TEST_CASE("type-keyed storage put/get/has/remove")
{
    LogSink sink;
    UserConnection connection{"a", sink, 1001};
    struct Foo {
        int value;
    };
    CHECK_FALSE(connection.has<Foo>());
    CHECK(connection.get<Foo>() == nullptr);

    connection.put(Foo{7});
    REQUIRE(connection.has<Foo>());
    REQUIRE(connection.get<Foo>() != nullptr);
    CHECK(connection.get<Foo>()->value == 7);

    connection.remove<Foo>();
    CHECK_FALSE(connection.has<Foo>());
}

TEST_CASE("remove_by_address forgets the connection and is idempotent")
{
    LogSink sink;
    ConnectionManager connections{1001, sink};
    connections.get_or_create("host:1");
    CHECK(connections.get("host:1") != nullptr);
    connections.remove_by_address("host:1");
    CHECK(connections.get("host:1") == nullptr);
    connections.remove_by_address("host:1"); // no-op, must not crash
}
