/// Protocol infrastructure tests (Phase 2 + 3).

#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "endweave/codec/types/primitives.h"
#include "endweave/codec/wrapper.h"
#include "endweave/codec/writer.h"
#include "endweave/connection.h"
#include "endweave/debug.h"
#include "endweave/exception.h"
#include "endweave/pipeline.h"
#include "endweave/protocol/base.h"
#include "endweave/protocol/direction.h"
#include "endweave/protocol/manager.h"
#include "endweave/protocol/packet_ids.h"
#include "endweave/protocol/protocol.h"
#include "endweave/protocol/versions.h"

using namespace endweave;

// -- Direction --

TEST(Direction, Values) {
    EXPECT_NE(Direction::SERVERBOUND, Direction::CLIENTBOUND);
}

// -- PacketId --

TEST(PacketId, KnownValues) {
    EXPECT_EQ(static_cast<int>(PacketId::LOGIN), 1);
    EXPECT_EQ(static_cast<int>(PacketId::START_GAME), 11);
    EXPECT_EQ(static_cast<int>(PacketId::REQUEST_NETWORK_SETTINGS), 193);
    EXPECT_EQ(static_cast<int>(PacketId::LEVEL_SOUND_EVENT), 123);
    EXPECT_EQ(static_cast<int>(PacketId::SET_ACTOR_DATA), 39);
    EXPECT_EQ(static_cast<int>(PacketId::SYNC_WORLD_CLOCKS), 344);
}

TEST(PacketId, Label) {
    auto label = packet_label(11);
    EXPECT_NE(label.find("START_GAME"), std::string::npos);
    EXPECT_NE(label.find("11"), std::string::npos);
    EXPECT_NE(label.find("0x0B"), std::string::npos);

    // Unknown packet ID
    auto unknown = packet_label(999);
    EXPECT_NE(unknown.find("999"), std::string::npos);
}

// -- Protocol --

TEST(Protocol, RegisterAndTransform) {
    Protocol proto(924, 944, "test");
    EXPECT_EQ(proto.server_protocol(), 924);
    EXPECT_EQ(proto.client_protocol(), 944);
    EXPECT_EQ(proto.name(), "test");
    EXPECT_FALSE(proto.is_base());

    // Register a handler that reads a varint and writes it doubled
    proto.register_serverbound(11, [](PacketWrapper& w) {
        auto val = w.read<var_int>();
        if (val) w.write<var_int>(*val * 2);
    });

    EXPECT_TRUE(proto.has_handler_or_cancel(Direction::SERVERBOUND, 11));
    EXPECT_FALSE(proto.has_handler_or_cancel(Direction::SERVERBOUND, 12));
    EXPECT_FALSE(proto.has_handler_or_cancel(Direction::CLIENTBOUND, 11));

    // Transform
    PacketWriter pw;
    pw.write_varint(21);
    auto buf = pw.to_string();

    PacketWrapper wrapper(buf);
    proto.transform(Direction::SERVERBOUND, 11, wrapper);
    auto output = wrapper.to_string();

    PacketReader verify(output);
    EXPECT_EQ(*verify.read_varint(), 42);
}

TEST(Protocol, CancelPacket) {
    Protocol proto(924, 944);
    proto.cancel_serverbound({100, 101, 102});

    EXPECT_TRUE(proto.has_handler_or_cancel(Direction::SERVERBOUND, 100));
    EXPECT_TRUE(proto.has_handler_or_cancel(Direction::SERVERBOUND, 101));
    EXPECT_FALSE(proto.has_handler_or_cancel(Direction::SERVERBOUND, 99));

    PacketWrapper wrapper("");
    proto.transform(Direction::SERVERBOUND, 100, wrapper);
    EXPECT_TRUE(wrapper.cancelled());
}

TEST(Protocol, DefaultName) {
    Protocol proto(924, 944);
    EXPECT_EQ(proto.name(), "924->944");
}

// -- ProtocolManager --

TEST(ProtocolManager, DirectLookup) {
    Protocol p1(924, 944, "v924_to_v944");
    Protocol p2(944, 924, "v944_to_v924");

    ProtocolManager mgr;
    mgr.register_protocol(&p1);
    mgr.register_protocol(&p2);

    EXPECT_EQ(mgr.get(924, 944), &p1);
    EXPECT_EQ(mgr.get(944, 924), &p2);
    EXPECT_EQ(mgr.get(924, 898), nullptr);
}

TEST(ProtocolManager, SameVersion) {
    ProtocolManager mgr;
    auto* path = mgr.get_path(924, 924);
    ASSERT_NE(path, nullptr);
    EXPECT_TRUE(path->empty());
}

TEST(ProtocolManager, DirectPath) {
    Protocol p(924, 944);
    ProtocolManager mgr;
    mgr.register_protocol(&p);

    auto* path = mgr.get_path(924, 944);
    ASSERT_NE(path, nullptr);
    ASSERT_EQ(path->size(), 1u);
    EXPECT_EQ((*path)[0], &p);
}

TEST(ProtocolManager, BfsChaining) {
    // 944 -> 924 -> 898: two-step chain
    Protocol p1(924, 944, "v944_to_v924");
    Protocol p2(898, 924, "v924_to_v898");

    ProtocolManager mgr;
    mgr.register_protocol(&p1);
    mgr.register_protocol(&p2);

    auto* path = mgr.get_path(898, 944);
    ASSERT_NE(path, nullptr);
    ASSERT_EQ(path->size(), 2u);
    EXPECT_EQ((*path)[0]->name(), "v944_to_v924");
    EXPECT_EQ((*path)[1]->name(), "v924_to_v898");
}

TEST(ProtocolManager, NoPath) {
    Protocol p(924, 944);
    ProtocolManager mgr;
    mgr.register_protocol(&p);

    auto* path = mgr.get_path(860, 944);
    EXPECT_EQ(path, nullptr);
}

TEST(ProtocolManager, BaseProtocols) {
    Protocol base(924, 0, "base", true);
    ProtocolManager mgr;
    mgr.register_base(&base);

    EXPECT_EQ(mgr.base_protocols().size(), 1u);
    EXPECT_EQ(mgr.base_protocols()[0], &base);
}

TEST(ProtocolManager, SupportedVersions) {
    Protocol p1(924, 944);
    Protocol p2(898, 924);

    ProtocolManager mgr;
    mgr.register_protocol(&p1);
    mgr.register_protocol(&p2);

    auto versions = mgr.get_supported_versions(898);
    EXPECT_NE(std::find(versions.begin(), versions.end(), 898), versions.end());
    EXPECT_NE(std::find(versions.begin(), versions.end(), 924), versions.end());
    EXPECT_NE(std::find(versions.begin(), versions.end(), 944), versions.end());
}

// -- ProtocolVersion --

TEST(ProtocolVersion, Registry) {
    auto& versions = get_versions();
    EXPECT_EQ(versions.at(924)->minecraft_version, "1.26.0");
    EXPECT_EQ(versions.at(944)->minecraft_version, "1.26.10");
    EXPECT_EQ(versions.at(859)->minecraft_version, "1.21.120");
}

// -- UserConnection --

TEST(UserConnection, BasicProperties) {
    UserConnection conn("127.0.0.1:19132", 924);
    EXPECT_EQ(conn.address(), "127.0.0.1:19132");
    EXPECT_EQ(conn.server_protocol(), 924);
    EXPECT_EQ(conn.client_protocol(), 0);
    EXPECT_FALSE(conn.needs_translation());

    conn.set_client_protocol(944);
    EXPECT_TRUE(conn.needs_translation());

    conn.set_client_protocol(924);
    EXPECT_FALSE(conn.needs_translation());
}

TEST(UserConnection, TypeKeyedStorage) {
    struct MyState { int value = 42; };
    struct OtherState { std::string name = "test"; };

    UserConnection conn("addr", 924);
    EXPECT_FALSE(conn.has<MyState>());
    EXPECT_EQ(conn.get<MyState>(), nullptr);

    conn.put(MyState{99});
    EXPECT_TRUE(conn.has<MyState>());
    EXPECT_EQ(conn.get<MyState>()->value, 99);

    conn.put(OtherState{"hello"});
    EXPECT_EQ(conn.get<OtherState>()->name, "hello");

    conn.remove<MyState>();
    EXPECT_FALSE(conn.has<MyState>());
    EXPECT_TRUE(conn.has<OtherState>());

    conn.clear_storage();
    EXPECT_FALSE(conn.has<OtherState>());
}

// -- ConnectionManager --

TEST(ConnectionManager, GetOrCreate) {
    ConnectionManager mgr(924);
    auto& c1 = mgr.get_or_create("addr1");
    EXPECT_EQ(c1.server_protocol(), 924);

    auto& c2 = mgr.get_or_create("addr1");
    EXPECT_EQ(&c1, &c2); // same object

    auto& c3 = mgr.get_or_create("addr2");
    EXPECT_NE(&c1, &c3);
}

TEST(ConnectionManager, GetAndRemove) {
    ConnectionManager mgr(924);
    EXPECT_EQ(mgr.get("addr"), nullptr);

    mgr.get_or_create("addr");
    EXPECT_NE(mgr.get("addr"), nullptr);

    mgr.remove("addr");
    EXPECT_EQ(mgr.get("addr"), nullptr);
}

// -- Base Protocol --

TEST(BaseProtocol, DetectsClientProtocol) {
    auto base = create_base_protocol(924);
    UserConnection conn("addr", 924);

    // Simulate RequestNetworkSettings packet with protocol 944 (big-endian int)
    PacketWriter pw;
    pw.write_int_be(944);
    pw.write_bytes("trailing"); // trailing data
    auto buf = pw.to_string();

    PacketWrapper wrapper(buf, &conn);
    base->transform(Direction::SERVERBOUND, static_cast<int>(PacketId::REQUEST_NETWORK_SETTINGS), wrapper);

    EXPECT_EQ(conn.client_protocol(), 944);

    // Output should have server protocol (924) in big-endian + trailing
    auto output = wrapper.to_string();
    PacketReader verify(output);
    EXPECT_EQ(*verify.read_int_be(), 924);
    auto trailing = verify.read_remaining();
    EXPECT_EQ(trailing, "trailing");
}

TEST(BaseProtocol, RewritesLogin) {
    auto base = create_base_protocol(924);
    UserConnection conn("addr", 924);
    conn.set_client_protocol(944);

    PacketWriter pw;
    pw.write_int_be(944); // client version
    pw.write_bytes("login_data");
    auto buf = pw.to_string();

    PacketWrapper wrapper(buf, &conn);
    base->transform(Direction::SERVERBOUND, static_cast<int>(PacketId::LOGIN), wrapper);

    auto output = wrapper.to_string();
    PacketReader verify(output);
    EXPECT_EQ(*verify.read_int_be(), 924); // rewritten to server protocol
}

// -- DebugHandler --

TEST(DebugHandler, Filtering) {
    DebugHandler disabled;
    EXPECT_FALSE(disabled.should_log(11));

    DebugHandler all_enabled(true);
    EXPECT_TRUE(all_enabled.should_log(11));
    EXPECT_TRUE(all_enabled.should_log(123));

    DebugHandler filtered(true, {11, 39});
    EXPECT_TRUE(filtered.should_log(11));
    EXPECT_TRUE(filtered.should_log(39));
    EXPECT_FALSE(filtered.should_log(123));
}

TEST(DebugHandler, TransformPhases) {
    DebugHandler h(true, {}, true, false);
    EXPECT_TRUE(h.log_pre_packet_transform());
    EXPECT_FALSE(h.log_post_packet_transform());

    DebugHandler h2(true, {}, true, true);
    EXPECT_TRUE(h2.log_post_packet_transform());
}

// -- InformativeException --

TEST(InformativeException, Message) {
    InformativeException err("NullPointerException: oops");
    err.set("Direction", "SERVERBOUND")
       .set("Packet ID", "START_GAME(11)");

    auto msg = err.message();
    EXPECT_NE(msg.find("Direction: SERVERBOUND"), std::string::npos);
    EXPECT_NE(msg.find("Packet ID: START_GAME(11)"), std::string::npos);
    EXPECT_NE(msg.find("NullPointerException: oops"), std::string::npos);
    EXPECT_NE(msg.find("Endweave"), std::string::npos);
}

// -- ProtocolPipeline --

TEST(Pipeline, EndToEndTranslation) {
    // Simulate: v944 client connecting to v924 server
    auto base = create_base_protocol(924);

    // Create v924->v944 protocol (for translating to v944 client)
    Protocol v924_to_v944_proto(924, 944, "v924_to_v944");
    // Register a simple clientbound handler that maps network_block_pos to block_pos
    v924_to_v944_proto.register_clientbound(
        static_cast<int>(PacketId::UPDATE_BLOCK),
        [](PacketWrapper& w) { (void)w.map<network_block_pos, block_pos>(); });

    ProtocolManager mgr;
    mgr.register_base(base.get());
    mgr.register_protocol(&v924_to_v944_proto);

    ConnectionManager conns(924);
    ProtocolPipeline pipeline(mgr, conns);

    // Step 1: Client sends RequestNetworkSettings with protocol 944
    PacketWriter rns;
    rns.write_int_be(944);
    auto rns_buf = rns.to_string();
    std::string rns_out;
    bool cancelled = false;

    pipeline.on_packet_receive("addr:1234",
        static_cast<int>(PacketId::REQUEST_NETWORK_SETTINGS),
        rns_buf, rns_out, cancelled);
    EXPECT_FALSE(cancelled);

    // Verify client protocol was detected
    auto* conn = conns.get("addr:1234");
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->client_protocol(), 944);

    // Step 1b: Send a Login packet to trigger pipeline caching
    // (pipeline is cached on the NEXT serverbound call after client_protocol is set)
    PacketWriter login;
    login.write_int_be(944); // protocol version
    auto login_buf = login.to_string();
    std::string login_out;
    pipeline.on_packet_receive("addr:1234",
        static_cast<int>(PacketId::LOGIN),
        login_buf, login_out, cancelled);

    // Step 2: Server sends UpdateBlock with NetworkBlockPos
    PacketWriter ub;
    ub.write<network_block_pos>({10, 64, -30});
    ub.write_bytes("trailing");
    auto ub_buf = ub.to_string();
    std::string ub_out;
    cancelled = false;

    bool modified = pipeline.on_packet_send("addr:1234",
        static_cast<int>(PacketId::UPDATE_BLOCK),
        ub_buf, ub_out, cancelled);
    EXPECT_TRUE(modified);
    EXPECT_FALSE(cancelled);

    // Verify output has BlockPos format
    PacketReader verify(ub_out);
    auto pos = verify.read<block_pos>();
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(std::get<0>(*pos), 10);
    EXPECT_EQ(std::get<1>(*pos), 64);
    EXPECT_EQ(std::get<2>(*pos), -30);
}

TEST(Pipeline, SameVersionNoTranslation) {
    auto base = create_base_protocol(924);
    ProtocolManager mgr;
    mgr.register_base(base.get());

    ConnectionManager conns(924);
    ProtocolPipeline pipeline(mgr, conns);

    // Client with same version
    PacketWriter rns;
    rns.write_int_be(924);
    auto rns_buf = rns.to_string();
    std::string out;
    bool cancelled = false;

    pipeline.on_packet_receive("addr:1234",
        static_cast<int>(PacketId::REQUEST_NETWORK_SETTINGS),
        rns_buf, out, cancelled);

    auto* conn = conns.get("addr:1234");
    ASSERT_NE(conn, nullptr);
    EXPECT_FALSE(conn->needs_translation());
}

TEST(Pipeline, CancelledPacket) {
    auto base = create_base_protocol(924);
    Protocol proto(924, 944);
    proto.cancel_serverbound({42});

    ProtocolManager mgr;
    mgr.register_base(base.get());
    mgr.register_protocol(&proto);

    ConnectionManager conns(924);
    ProtocolPipeline pipeline(mgr, conns);

    // Detect client
    PacketWriter rns;
    rns.write_int_be(944);
    auto rns_buf = rns.to_string();
    std::string out;
    bool cancelled = false;
    pipeline.on_packet_receive("addr:1234",
        static_cast<int>(PacketId::REQUEST_NETWORK_SETTINGS),
        rns_buf, out, cancelled);

    // Send cancelled packet
    std::string out2;
    bool cancelled2 = false;
    pipeline.on_packet_receive("addr:1234", 42, "data", out2, cancelled2);
    EXPECT_TRUE(cancelled2);
}

TEST(InformativeException, Truncation) {
    InformativeException err("test");
    std::string long_value(300, 'x');
    err.set("key", long_value);
    auto msg = err.message();
    EXPECT_NE(msg.find("..."), std::string::npos);
}
