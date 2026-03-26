"""Tests for SoundRewriter and v924-to-v944 sound remapping."""

from helpers import varint_bytes, write_actor_data_list

from endstone_endweave.codec import (
    ACTOR_DATA_LIST,
    PacketWrapper,
)
from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.protocol.sound_rewriter import SoundRewriter
from endstone_endweave.protocol.v924_to_v944.protocol import _remap_sound

# ---------------------------------------------------------------------------
# Tests: _remap_sound (v924 -> v944)
# ---------------------------------------------------------------------------


class TestRemapSound:
    def test_below_threshold(self):
        assert _remap_sound(0) == 0
        assert _remap_sound(596) == 596

    def test_at_threshold(self):
        assert _remap_sound(597) == 599

    def test_above_threshold(self):
        assert _remap_sound(598) == 600
        assert _remap_sound(1000) == 1002


# ---------------------------------------------------------------------------
# Tests: SoundRewriter handlers
# ---------------------------------------------------------------------------


class TestSoundRewriter:
    def setup_method(self):
        self.rewriter = SoundRewriter(
            sound_remap=_remap_sound,
            actor_data_int_remappers={126: _remap_sound},
        )

    def test_level_sound_event_remapped(self):
        w = PacketWriter()
        w.write_uvarint(597)  # Event ID (at threshold)
        w.write_bytes(b"\xaa\xbb")  # trailing (Position, ActorType, etc.)
        wrapper = PacketWrapper(w.to_bytes())
        self.rewriter.rewrite_level_sound_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint() == 599  # remapped
        assert r.read_remaining() == b"\xaa\xbb"

    def test_level_sound_event_unchanged(self):
        w = PacketWriter()
        w.write_uvarint(100)  # below threshold
        w.write_bytes(b"\xcc")
        wrapper = PacketWrapper(w.to_bytes())
        self.rewriter.rewrite_level_sound_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint() == 100
        assert r.read_remaining() == b"\xcc"

    def test_actor_data_heartbeat_remapped(self):
        """ActorData key 126 (heartbeat) with int type (2) gets remapped."""
        w = PacketWriter()
        w.write_uvarint64(1)  # Target Runtime ID
        # ActorData list: 1 entry with key=126, type=2 (int), value=597
        write_actor_data_list(w, [(126, 2, varint_bytes(597))])
        w.write_bytes(b"\xdd")  # trailing (Synched Properties, Tick)
        wrapper = PacketWrapper(w.to_bytes())
        self.rewriter.rewrite_set_actor_data(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint64() == 1
        entries = ACTOR_DATA_LIST.read(r)
        assert len(entries) == 1
        assert entries[0].key == 126
        assert entries[0].type_id == 2
        assert entries[0].value == 599  # remapped from 597
        assert r.read_remaining() == b"\xdd"

    def test_actor_data_non_matching_key_unchanged(self):
        """ActorData with non-heartbeat key is not remapped."""
        w = PacketWriter()
        w.write_uvarint64(1)
        write_actor_data_list(w, [(99, 2, varint_bytes(597))])
        w.write_bytes(b"\xee")
        wrapper = PacketWrapper(w.to_bytes())
        self.rewriter.rewrite_set_actor_data(wrapper)
        r = PacketReader(wrapper.to_bytes())
        r.read_uvarint64()
        entries = ACTOR_DATA_LIST.read(r)
        assert len(entries) == 1
        assert entries[0].value == 597  # unchanged
