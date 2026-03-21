"""Tests for PacketWrapper and Type system."""

import struct
from unittest.mock import MagicMock


from endstone_endweave.codec import (
    PacketWrapper,
    BYTE,
    BOOL,
    INT_BE,
    INT_LE,
    VAR_INT,
    UVAR_INT,
    STRING,
    REMAINING_BYTES,
    FLOAT_LE,
    LONG_LE,
    SHORT_LE,
    USHORT_LE,
    UINT_LE,
    UVAR_INT64,
    VAR_INT64,
    UUID,
)
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.connection import UserConnection
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.direction import Direction


class TestPacketWrapperBasics:
    def test_passthrough_preserves_data(self):
        """passthrough reads and writes, returning the value."""
        w = PacketWriter()
        w.write_int_be(944)
        w.write_string("hello")
        payload = w.to_bytes()

        wrapper = PacketWrapper(payload)
        val = wrapper.passthrough(INT_BE)
        assert val == 944
        s = wrapper.passthrough(STRING)
        assert s == "hello"
        assert wrapper.to_bytes() == payload

    def test_read_removes_field(self):
        """read consumes from input without writing to output."""
        w = PacketWriter()
        w.write_int_be(944)
        w.write_string("hello")
        payload = w.to_bytes()

        wrapper = PacketWrapper(payload)
        val = wrapper.read(INT_BE)
        assert val == 944
        wrapper.passthrough(STRING)
        result = wrapper.to_bytes()
        w2 = PacketWriter()
        w2.write_string("hello")
        assert result == w2.to_bytes()

    def test_write_inserts_field(self):
        """write appends to output without reading from input."""
        w = PacketWriter()
        w.write_string("hello")
        payload = w.to_bytes()

        wrapper = PacketWrapper(payload)
        wrapper.write(INT_BE, 999)
        wrapper.passthrough(STRING)
        result = wrapper.to_bytes()

        w2 = PacketWriter()
        w2.write_int_be(999)
        w2.write_string("hello")
        assert result == w2.to_bytes()

    def test_cancel(self):
        wrapper = PacketWrapper(b"\x00")
        assert not wrapper.cancelled
        wrapper.cancel()
        assert wrapper.cancelled

    def test_passthrough_all(self):
        payload = b"\x01\x02\x03\x04"
        wrapper = PacketWrapper(payload)
        wrapper.read(BYTE)  # consume first byte
        remaining = wrapper.passthrough_all()
        assert remaining == b"\x02\x03\x04"
        assert wrapper.to_bytes() == b"\x02\x03\x04"

    def test_to_bytes_includes_unread_tail(self):
        """to_bytes appends any unread input bytes automatically."""
        payload = b"\x01\x02\x03\x04"
        wrapper = PacketWrapper(payload)
        wrapper.passthrough(BYTE)
        result = wrapper.to_bytes()
        assert result == payload

    def test_has_remaining(self):
        wrapper = PacketWrapper(b"\x01\x02")
        assert wrapper.has_remaining()
        wrapper.read(BYTE)
        assert wrapper.has_remaining()
        wrapper.read(BYTE)
        assert not wrapper.has_remaining()

    def test_user(self):
        conn = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        wrapper = PacketWrapper(b"\x00", user=conn)
        assert wrapper.user() is conn


class TestPacketWrapperTransform:
    def test_remap_int_field(self):
        """Read a field, transform it, write the new value."""
        w = PacketWriter()
        w.write_int_be(944)
        w.write_bytes(b"\xde\xad")
        payload = w.to_bytes()

        wrapper = PacketWrapper(payload)
        old_val = wrapper.read(INT_BE)
        assert old_val == 944
        wrapper.write(INT_BE, 924)
        wrapper.passthrough_all()
        result = wrapper.to_bytes()

        assert struct.unpack(">i", result[:4])[0] == 924
        assert result[4:] == b"\xde\xad"

    def test_delete_middle_field(self):
        """Remove a field from the middle of a packet."""
        w = PacketWriter()
        w.write_byte(0x01)
        w.write_int_le(42)
        w.write_byte(0x02)
        payload = w.to_bytes()

        wrapper = PacketWrapper(payload)
        wrapper.passthrough(BYTE)
        wrapper.read(INT_LE)
        wrapper.passthrough(BYTE)
        result = wrapper.to_bytes()
        assert result == bytes([0x01, 0x02])

    def test_insert_field_in_middle(self):
        """Insert a new field between existing ones."""
        w = PacketWriter()
        w.write_byte(0x01)
        w.write_byte(0x02)
        payload = w.to_bytes()

        wrapper = PacketWrapper(payload)
        wrapper.passthrough(BYTE)
        wrapper.write(BOOL, True)
        wrapper.passthrough(BYTE)
        result = wrapper.to_bytes()
        assert result == bytes([0x01, 0x01, 0x02])


class TestTypeRoundtrips:
    """Verify each type reads what it writes through the wrapper."""

    def _roundtrip(self, field_type, value):
        w = PacketWriter()
        field_type.write(w, value)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        result = wrapper.passthrough(field_type)
        assert result == value
        assert wrapper.to_bytes() == payload

    def test_byte(self):
        self._roundtrip(BYTE, 0xFF)

    def test_bool_true(self):
        self._roundtrip(BOOL, True)

    def test_bool_false(self):
        self._roundtrip(BOOL, False)

    def test_short_le(self):
        self._roundtrip(SHORT_LE, -1234)

    def test_ushort_le(self):
        self._roundtrip(USHORT_LE, 65535)

    def test_int_le(self):
        self._roundtrip(INT_LE, -100000)

    def test_int_be(self):
        self._roundtrip(INT_BE, 924)

    def test_uint_le(self):
        self._roundtrip(UINT_LE, 0xDEADBEEF)

    def test_long_le(self):
        self._roundtrip(LONG_LE, 1234567890123)

    def test_float_le(self):
        w = PacketWriter()
        FLOAT_LE.write(w, 3.14)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        result = wrapper.passthrough(FLOAT_LE)
        assert abs(result - 3.14) < 0.001

    def test_var_int(self):
        for val in [0, 1, -1, 42, -42, 2147483647, -2147483648]:
            self._roundtrip(VAR_INT, val)

    def test_uvar_int(self):
        for val in [0, 1, 127, 128, 300, 0xFFFFFFFF]:
            self._roundtrip(UVAR_INT, val)

    def test_var_long(self):
        for val in [0, 1, -1, 2147483647]:
            self._roundtrip(VAR_INT64, val)

    def test_uvar_long(self):
        for val in [0, 1, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF]:
            self._roundtrip(UVAR_INT64, val)

    def test_string(self):
        self._roundtrip(STRING, "hello world")

    def test_string_empty(self):
        self._roundtrip(STRING, "")

    def test_uuid(self):
        self._roundtrip(UUID, b"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10")

    def test_remaining_bytes(self):
        self._roundtrip(REMAINING_BYTES, b"\xde\xad\xbe\xef")


class TestWrapperHandlerIntegration:
    """Test wrapper-style handlers integrated with Protocol."""

    def test_wrapper_handler_rewrites_packet(self):
        def rewrite_protocol(wrapper: PacketWrapper) -> None:
            conn = wrapper.user()
            wrapper.read(INT_BE)
            wrapper.write(INT_BE, conn.server_protocol)

        p = Protocol(server_protocol=924, client_protocol=944)
        p.register_serverbound(193, rewrite_protocol)

        conn = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )

        w = PacketWriter()
        w.write_int_be(944)
        w.write_bytes(b"\xde\xad")
        payload = w.to_bytes()

        wrapper = PacketWrapper(payload, user=conn)
        p.transform(Direction.SERVERBOUND, 193, wrapper)
        assert not wrapper.cancelled
        result = wrapper.to_bytes()
        assert struct.unpack(">i", result[:4])[0] == 924
        assert result[4:] == b"\xde\xad"

    def test_wrapper_handler_cancel(self):
        def cancel_handler(wrapper: PacketWrapper) -> None:
            wrapper.cancel()

        p = Protocol(server_protocol=924, client_protocol=944)
        p.register_serverbound(42, cancel_handler)

        conn = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        wrapper = PacketWrapper(b"\x00", user=conn)
        p.transform(Direction.SERVERBOUND, 42, wrapper)
        assert wrapper.cancelled

    def test_wrapper_handler_passthrough_unchanged(self):
        def noop_handler(wrapper: PacketWrapper) -> None:
            wrapper.passthrough_all()

        p = Protocol(server_protocol=924, client_protocol=944)
        p.register_serverbound(42, noop_handler)

        conn = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        wrapper = PacketWrapper(b"\x01\x02\x03", user=conn)
        p.transform(Direction.SERVERBOUND, 42, wrapper)
        assert not wrapper.cancelled
        assert wrapper.to_bytes() == b"\x01\x02\x03"
