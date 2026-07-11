#include "endweave/protocol/manager.h"
#include "endweave/protocol/protocol.h"
#include "endweave/protocol/v1001_to_v975.h"
#include "endweave/protocol/v975_to_v1001.h"

#include <catch2/catch_test_macros.hpp>
#include <vector>

using endweave::Protocol;
using endweave::ProtocolManager;

TEST_CASE("get_path resolves direct pairs, same-version, and unreachable")
{
    auto up = endweave::v975_to_v1001::create_protocol();   // server 975, client 1001
    auto down = endweave::v1001_to_v975::create_protocol(); // server 1001, client 975
    ProtocolManager manager;
    manager.register_protocol(up);
    manager.register_protocol(down);

    auto downgrade = manager.get_path(1001, 975);
    REQUIRE(downgrade.has_value());
    REQUIRE(downgrade->size() == 1);
    CHECK((*downgrade)[0] == &down);

    auto upgrade = manager.get_path(975, 1001);
    REQUIRE(upgrade.has_value());
    REQUIRE(upgrade->size() == 1);
    CHECK((*upgrade)[0] == &up);

    auto same = manager.get_path(1001, 1001);
    REQUIRE(same.has_value());
    CHECK(same->empty());

    CHECK_FALSE(manager.get_path(1001, 500).has_value());

    CHECK(manager.get(1001, 975) == &down);
    CHECK(manager.get(1001, 500) == nullptr);
}

TEST_CASE("get_path chains multiple hops via BFS")
{
    Protocol a{1030, 1001}; // server 1030, client 1001
    Protocol b{1001, 975};  // server 1001, client 975
    ProtocolManager manager;
    manager.register_protocol(a);
    manager.register_protocol(b);

    auto path = manager.get_path(1030, 975); // client 975 -> server 1030
    REQUIRE(path.has_value());
    REQUIRE(path->size() == 2);
    CHECK((*path)[0] == &b); // client 975 -> server 1001
    CHECK((*path)[1] == &a); // client 1001 -> server 1030

    CHECK(manager.get_supported_versions(1030) == std::vector<int>{975, 1001, 1030});
}

TEST_CASE("registering a protocol invalidates the negative path cache")
{
    Protocol b{1001, 975};
    ProtocolManager manager;
    manager.register_protocol(b);
    CHECK_FALSE(manager.get_path(1030, 975).has_value()); // caches nullopt

    Protocol a{1030, 1001};
    manager.register_protocol(a); // must clear the cached nullopt
    auto path = manager.get_path(1030, 975);
    REQUIRE(path.has_value());
    CHECK(path->size() == 2);
}
