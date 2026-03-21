"""Tests for per-connection state storage."""

from unittest.mock import MagicMock

from endstone_endweave.connection import (
    ConnectionState,
    ConnectionManager,
    UserConnection,
)


class TestUserConnectionStorage:
    def test_put_and_get(self):
        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock())

        class MyState:
            value = 42

        state = MyState()
        conn.put(state)
        assert conn.get(MyState) is state
        assert conn.get(MyState).value == 42

    def test_has(self):
        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock())

        class Tracker:
            pass

        assert not conn.has(Tracker)
        conn.put(Tracker())
        assert conn.has(Tracker)

    def test_remove(self):
        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock())

        class Tracker:
            pass

        conn.put(Tracker())
        conn.remove(Tracker)
        assert not conn.has(Tracker)
        assert conn.get(Tracker) is None

    def test_remove_nonexistent_is_noop(self):
        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock())

        class Tracker:
            pass

        conn.remove(Tracker)  # should not raise

    def test_clear_storage(self):
        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock())

        class A:
            pass

        class B:
            pass

        conn.put(A())
        conn.put(B())
        conn.clear_storage()
        assert not conn.has(A)
        assert not conn.has(B)

    def test_get_returns_none_for_missing(self):
        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock())

        class Missing:
            pass

        assert conn.get(Missing) is None

    def test_put_overwrites_same_type(self):
        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock())

        class Counter:
            def __init__(self, n):
                self.n = n

        conn.put(Counter(1))
        conn.put(Counter(2))
        assert conn.get(Counter).n == 2


class TestConnectionManagerCleanup:
    def test_remove_by_address_clears_storage(self):
        mgr = ConnectionManager(server_protocol=924, logger=MagicMock())
        conn = mgr.get_or_create("1.2.3.4:1234")

        class Tracker:
            pass

        conn.put(Tracker())
        mgr.remove_by_address("1.2.3.4:1234")
        # Verify storage was cleared (accessing conn directly since we still hold ref)
        assert not conn.has(Tracker)


class TestConnectionState:
    def test_default_state_is_pre_login(self):
        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock())
        assert conn.state == ConnectionState.HANDSHAKE

    def test_state_transitions(self):
        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock())
        conn.state = ConnectionState.LOGIN
        assert conn.state == ConnectionState.LOGIN
        conn.state = ConnectionState.PLAY
        assert conn.state == ConnectionState.PLAY
