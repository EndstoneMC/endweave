"""Tests for SoundRewriter and all version-pair sound remapping."""

from helpers import varint_bytes, write_actor_data_list

from endstone_endweave.codec import (
    ACTOR_DATA_LIST,
    PacketWrapper,
)
from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.protocol.sound_rewriter import SoundRewriter
from endstone_endweave.protocol.v860_to_v898.protocol import _remap_sound as remap_v860_to_v898
from endstone_endweave.protocol.v898_to_v860.protocol import _remap_sound as remap_v898_to_v860
from endstone_endweave.protocol.v898_to_v924.protocol import _remap_sound as remap_v898_to_v924
from endstone_endweave.protocol.v924_to_v898.protocol import _remap_sound as remap_v924_to_v898
from endstone_endweave.protocol.v924_to_v944.protocol import _remap_sound as remap_v924_to_v944
from endstone_endweave.protocol.v944_to_v924.protocol import _remap_sound as remap_v944_to_v924

# ---------------------------------------------------------------------------
# Tests: _remap_sound (v860 -> v898)
# ---------------------------------------------------------------------------


class TestRemapSoundV860ToV898:
    def test_below_threshold(self):
        assert remap_v860_to_v898(0) == 0
        assert remap_v860_to_v898(565) == 565

    def test_at_threshold(self):
        # v860 Undefined (566) shifts to v898 Undefined (578)
        assert remap_v860_to_v898(566) == 578

    def test_above_threshold(self):
        assert remap_v860_to_v898(567) == 579
        assert remap_v860_to_v898(600) == 612


# ---------------------------------------------------------------------------
# Tests: _remap_sound (v898 -> v860)
# ---------------------------------------------------------------------------


class TestRemapSoundV898ToV860:
    def test_below_threshold(self):
        assert remap_v898_to_v860(0) == 0
        assert remap_v898_to_v860(565) == 565

    def test_new_sounds_collapse_to_v860_undefined(self):
        # v898 sounds 566-577 (new) collapse to v860 Undefined (566)
        for v in range(566, 578):
            assert remap_v898_to_v860(v) == 566

    def test_at_v898_undefined(self):
        # v898 Undefined (578) maps back to v860 Undefined (566)
        assert remap_v898_to_v860(578) == 566

    def test_above_v898_undefined(self):
        assert remap_v898_to_v860(579) == 567
        assert remap_v898_to_v860(600) == 588


# ---------------------------------------------------------------------------
# Tests: _remap_sound (v898 -> v924)
# ---------------------------------------------------------------------------


class TestRemapSoundV898ToV924:
    def test_below_threshold(self):
        assert remap_v898_to_v924(0) == 0
        assert remap_v898_to_v924(577) == 577

    def test_at_threshold(self):
        # v898 Undefined (578) shifts to v924 Undefined (597)
        assert remap_v898_to_v924(578) == 597

    def test_above_threshold(self):
        assert remap_v898_to_v924(579) == 598
        assert remap_v898_to_v924(600) == 619


# ---------------------------------------------------------------------------
# Tests: _remap_sound (v924 -> v898)
# ---------------------------------------------------------------------------


class TestRemapSoundV924ToV898:
    def test_below_threshold(self):
        assert remap_v924_to_v898(0) == 0
        assert remap_v924_to_v898(577) == 577

    def test_at_threshold(self):
        # v924 sounds >= 578 cap at v898 Undefined (578)
        assert remap_v924_to_v898(578) == 578

    def test_above_threshold(self):
        assert remap_v924_to_v898(597) == 578
        assert remap_v924_to_v898(1000) == 578


# ---------------------------------------------------------------------------
# Tests: _remap_sound (v924 -> v944)
# ---------------------------------------------------------------------------


class TestRemapSoundV924ToV944:
    def test_below_threshold(self):
        assert remap_v924_to_v944(0) == 0
        assert remap_v924_to_v944(596) == 596

    def test_at_threshold(self):
        assert remap_v924_to_v944(597) == 599

    def test_above_threshold(self):
        assert remap_v924_to_v944(598) == 600
        assert remap_v924_to_v944(1000) == 1002


# ---------------------------------------------------------------------------
# Tests: _remap_sound (v944 -> v924)
# ---------------------------------------------------------------------------


class TestRemapSoundV944ToV924:
    def test_below_threshold(self):
        assert remap_v944_to_v924(0) == 0
        assert remap_v944_to_v924(596) == 596

    def test_growth_events_collapse_to_v924_undefined(self):
        # v944 PauseGrowth(597) and ResetGrowth(598) -> v924 Undefined (597)
        assert remap_v944_to_v924(597) == 597
        assert remap_v944_to_v924(598) == 597

    def test_at_v944_undefined(self):
        # v944 Undefined (599) shifts back to v924 Undefined (597)
        assert remap_v944_to_v924(599) == 597

    def test_above_v944_undefined(self):
        assert remap_v944_to_v924(600) == 598
        assert remap_v944_to_v924(1000) == 998


# ---------------------------------------------------------------------------
# Tests: SoundRewriter handlers
# ---------------------------------------------------------------------------


class TestSoundRewriter:
    def setup_method(self):
        self.rewriter = SoundRewriter(
            sound_remap=remap_v924_to_v944,
            actor_data_int_remappers={126: remap_v924_to_v944},
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
