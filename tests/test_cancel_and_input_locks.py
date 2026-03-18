"""Tests for cancel_clientbound and UpdateClientInputLocksPacket handler."""

from __future__ import annotations

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.base import ProtocolTranslator
from endstone_endweave.protocol.v924_to_v944.handlers.client_input_locks import rewrite_client_input_locks
from endstone_endweave.session import PlayerSession


def _make_session() -> PlayerSession:
    return PlayerSession(address="1.2.3.4:1234", client_protocol=944, server_protocol=924)


class TestCancelClientbound:
    def test_cancel_returns_cancel_transformation(self):
        t = ProtocolTranslator(server_protocol=924, client_protocol=944)
        t.cancel_clientbound(42)
        result = t.translate_clientbound(42, b"\x00", _make_session())
        assert result.cancel is True

    def test_cancel_does_not_affect_other_packets(self):
        t = ProtocolTranslator(server_protocol=924, client_protocol=944)
        t.cancel_clientbound(42)
        result = t.translate_clientbound(43, b"\x00", _make_session())
        assert result.cancel is False
        assert result.new_payload is None

    def test_cancel_multiple_ids(self):
        t = ProtocolTranslator(server_protocol=924, client_protocol=944)
        t.cancel_clientbound(10, 20, 30)
        for pid in (10, 20, 30):
            assert t.translate_clientbound(pid, b"", _make_session()).cancel is True

    def test_cancel_takes_priority_over_handler(self):
        t = ProtocolTranslator(server_protocol=924, client_protocol=944)
        t.register_clientbound(42, lambda p, s: None)
        t.cancel_clientbound(42)
        result = t.translate_clientbound(42, b"\x00", _make_session())
        assert result.cancel is True


class TestClientInputLocks:
    def test_strips_vec3(self):
        """v924 payload: varint(flags) + 3x float LE -> v944: varint(flags) only."""
        w = PacketWriter()
        w.write_varint(7)  # lock flags
        w.write_float_le(1.0)
        w.write_float_le(2.0)
        w.write_float_le(3.0)
        payload = w.to_bytes()

        result = rewrite_client_input_locks(payload, _make_session())
        assert result.new_payload is not None

        reader = PacketReader(result.new_payload)
        assert reader.read_varint() == 7
        assert reader.read_remaining() == b""

    def test_preserves_large_varint(self):
        """Ensure multi-byte varint lock flags are preserved correctly."""
        w = PacketWriter()
        w.write_varint(0xFFFF)  # large flags value
        w.write_float_le(0.0)
        w.write_float_le(0.0)
        w.write_float_le(0.0)
        payload = w.to_bytes()

        result = rewrite_client_input_locks(payload, _make_session())
        reader = PacketReader(result.new_payload)
        assert reader.read_varint() == 0xFFFF
        assert reader.read_remaining() == b""
