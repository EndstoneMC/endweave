"""Tests for enum remapping tables."""

from __future__ import annotations

from endstone_endweave.protocol.v924_to_v944.enum_map import (
    note_block_instrument_924_to_944,
    note_block_instrument_944_to_924,
)


class TestNoteBlockInstrument:
    """Tests NoteBlockInstrument displacement mapping."""

    def test_924_to_944_below_16_unchanged(self):
        """Values 0-15 should pass through unchanged."""
        for v in range(16):
            assert note_block_instrument_924_to_944(v) == v

    def test_924_to_944_shifts_mob_instruments(self):
        """v924 Zombie=16 → v944 Zombie=20, etc."""
        assert note_block_instrument_924_to_944(16) == 20  # Zombie
        assert note_block_instrument_924_to_944(17) == 21  # Skeleton
        assert note_block_instrument_924_to_944(18) == 22  # Creeper
        assert note_block_instrument_924_to_944(19) == 23  # Dragon
        assert note_block_instrument_924_to_944(20) == 24  # WitherSkeleton
        assert note_block_instrument_924_to_944(21) == 25  # Piglin

    def test_944_to_924_below_16_unchanged(self):
        """Values 0-15 should pass through unchanged."""
        for v in range(16):
            assert note_block_instrument_944_to_924(v) == v

    def test_944_to_924_trumpet_returns_none(self):
        """Trumpet variants (16-19) don't exist in v924."""
        for v in range(16, 20):
            assert note_block_instrument_944_to_924(v) is None

    def test_944_to_924_shifts_mob_instruments_back(self):
        """v944 Zombie=20 → v924 Zombie=16, etc."""
        assert note_block_instrument_944_to_924(20) == 16  # Zombie
        assert note_block_instrument_944_to_924(21) == 17  # Skeleton
        assert note_block_instrument_944_to_924(22) == 18  # Creeper
        assert note_block_instrument_944_to_924(23) == 19  # Dragon
        assert note_block_instrument_944_to_924(24) == 20  # WitherSkeleton
        assert note_block_instrument_944_to_924(25) == 21  # Piglin

    def test_roundtrip(self):
        """v924 → v944 → v924 should be identity for all v924 values."""
        for v in range(22):  # v924 range: 0-21
            mapped = note_block_instrument_924_to_944(v)
            back = note_block_instrument_944_to_924(mapped)
            assert back == v, f"Roundtrip failed for {v}: {v} → {mapped} → {back}"
