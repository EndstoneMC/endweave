"""Tests for multi-step protocol chain execution and init hooks."""

import struct
from unittest.mock import MagicMock

from endstone_endweave.codec import BYTE
from endstone_endweave.pipeline import ProtocolPipeline
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.base import create_base_protocol
from endstone_endweave.protocol.manager import ProtocolManager
from endstone_endweave.connection import ConnectionManager


def _make_event(packet_id, payload, address="1.2.3.4:1234"):
    event = MagicMock()
    event.packet_id = packet_id
    event.payload = payload
    event.address = address
    return event


class TestMultiStepChain:
    """Register two fake protocols (A->B, B->C) and verify BFS chains them."""

    def test_bfs_resolves_two_step_chain(self):
        mgr = ProtocolManager()
        p_ab = Protocol(server_protocol=100, client_protocol=200, name="a_to_b")
        p_bc = Protocol(server_protocol=200, client_protocol=300, name="b_to_c")
        mgr.register(p_ab)
        mgr.register(p_bc)
        chain = mgr.get_path(100, 300)
        assert chain is not None
        assert len(chain) == 2
        assert chain[0] is p_bc  # client 300 -> server 200
        assert chain[1] is p_ab  # client 200 -> server 100

    def test_chain_transforms_execute_in_sequence(self):
        """Two protocols each append a byte. Final payload should have both."""
        mgr = ProtocolManager()

        def append_0xAA(wrapper):
            wrapper.passthrough_all()
            wrapper.write(BYTE, 0xAA)

        def append_0xBB(wrapper):
            wrapper.passthrough_all()
            wrapper.write(BYTE, 0xBB)

        p_ab = Protocol(server_protocol=100, client_protocol=200, name="a_to_b")
        p_ab.register_clientbound(42, append_0xAA)

        p_bc = Protocol(server_protocol=200, client_protocol=300, name="b_to_c")
        p_bc.register_clientbound(42, append_0xBB)

        mgr.register(p_ab)
        mgr.register(p_bc)
        mgr.register_base(create_base_protocol(100))

        connections = ConnectionManager(server_protocol=100, logger=MagicMock())
        pipeline = ProtocolPipeline(mgr, connections, MagicMock())

        # Detect client protocol via RequestNetworkSettings
        rns_payload = struct.pack(">i", 300)
        rns_event = _make_event(193, rns_payload)
        pipeline.on_packet_receive(rns_event)

        # Trigger chain resolution (sets connection.active = True)
        sb_event = _make_event(99, b"\x00")
        pipeline.on_packet_receive(sb_event)

        # Now send a clientbound packet that both protocols rewrite
        event = _make_event(42, b"\x01")
        pipeline.on_packet_send(event)

        # Chain is [p_bc, p_ab], clientbound runs reversed: p_ab first, then p_bc
        # p_ab appends 0xAA -> b"\x01\xAA"
        # p_bc appends 0xBB -> b"\x01\xAA\xBB"
        assert event.payload == b"\x01\xaa\xbb"

    def test_serverbound_chain_order(self):
        """Serverbound applies chain in order (same wrapper shared across steps).

        Each protocol reads a field and writes a transformed field.
        Chain is [p_bc, p_ab], so p_bc processes first, then p_ab.
        """
        mgr = ProtocolManager()

        # p_bc: passthrough first byte, append 0xCC
        def sb_bc(wrapper):
            wrapper.passthrough(BYTE)
            wrapper.write(BYTE, 0xCC)

        # p_ab: passthrough everything so far, append 0xDD
        def sb_ab(wrapper):
            wrapper.passthrough_all()
            wrapper.write(BYTE, 0xDD)

        p_ab = Protocol(server_protocol=100, client_protocol=200, name="a_to_b")
        p_ab.register_serverbound(42, sb_ab)

        p_bc = Protocol(server_protocol=200, client_protocol=300, name="b_to_c")
        p_bc.register_serverbound(42, sb_bc)

        mgr.register(p_ab)
        mgr.register(p_bc)
        mgr.register_base(create_base_protocol(100))

        connections = ConnectionManager(server_protocol=100, logger=MagicMock())
        pipeline = ProtocolPipeline(mgr, connections, MagicMock())

        rns_payload = struct.pack(">i", 300)
        rns_event = _make_event(193, rns_payload)
        pipeline.on_packet_receive(rns_event)

        event = _make_event(42, b"\x01")
        pipeline.on_packet_receive(event)

        # Chain [p_bc, p_ab], serverbound: p_bc first, then p_ab
        # p_bc: passthrough 0x01, append 0xCC -> writer has b"\x01\xCC"
        # p_ab: passthrough_all (nothing left in reader), append 0xDD -> writer has b"\x01\xCC\xDD"
        assert event.payload == b"\x01\xcc\xdd"


class TestProtocolInitHook:
    def test_init_called_once_per_protocol_in_chain(self):
        mgr = ProtocolManager()

        init_calls = []

        class TrackingProtocol(Protocol):
            def init(self, connection):
                init_calls.append((self.name, connection.address))

        p1 = TrackingProtocol(server_protocol=100, client_protocol=200, name="p1")
        p2 = TrackingProtocol(server_protocol=200, client_protocol=300, name="p2")
        mgr.register(p1)
        mgr.register(p2)
        mgr.register_base(create_base_protocol(100))

        connections = ConnectionManager(server_protocol=100, logger=MagicMock())
        pipeline = ProtocolPipeline(mgr, connections, MagicMock())

        rns_payload = struct.pack(">i", 300)
        rns_event = _make_event(193, rns_payload)
        pipeline.on_packet_receive(rns_event)

        # Trigger chain resolution via a subsequent packet
        event = _make_event(42, b"\x00")
        pipeline.on_packet_receive(event)

        assert len(init_calls) == 2
        # Both protocols should have been initialized for this connection
        names = {c[0] for c in init_calls}
        assert names == {"p1", "p2"}

    def test_init_not_called_again_on_second_packet(self):
        mgr = ProtocolManager()
        init_count = [0]

        class CountingProtocol(Protocol):
            def init(self, connection):
                init_count[0] += 1

        p = CountingProtocol(server_protocol=924, client_protocol=944, name="cnt")
        mgr.register(p)
        mgr.register_base(create_base_protocol(924))

        connections = ConnectionManager(server_protocol=924, logger=MagicMock())
        pipeline = ProtocolPipeline(mgr, connections, MagicMock())

        rns_payload = struct.pack(">i", 944)
        rns_event = _make_event(193, rns_payload)
        pipeline.on_packet_receive(rns_event)

        for _ in range(5):
            event = _make_event(42, b"\x00")
            pipeline.on_packet_receive(event)

        assert init_count[0] == 1
