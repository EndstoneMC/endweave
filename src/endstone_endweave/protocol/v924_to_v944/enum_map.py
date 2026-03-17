"""Enum remapping tables for v924 <-> v944 translation.

These tables map enum values that changed between protocol versions.
Populated from changelog analysis. New enum values in v944 that don't
exist in v924 need to be mapped to the closest server equivalent.
"""

from __future__ import annotations

# InventoryLayout: v924 and v944 use the same numeric values (0-3),
# only the symbolic names differ (e.g. Survival->InventoryOnly).
# No runtime remapping needed.

# ActorType: new mob base IDs in v944 that don't exist in v924.
# These are composite bitmask IDs; we store only the base IDs (149-152).
# Nautilus=149, ZombieNautilus=150, Parched=151, CamelHusk=152
NEW_ACTOR_BASE_IDS_944: frozenset[int] = frozenset({149, 150, 151, 152})

# Enchant::Type - Lunge (41) is new in v944, not in v924
NEW_ENCHANT_IDS_944: frozenset[int] = frozenset({41})

# NoteBlockInstrument: trumpet variants (16-19) inserted in v944,
# pushing mob instruments up by 4.
# Clientbound (v924->v944): n >= 16 -> n + 4
# Serverbound (v944->v924): 16-19 -> None (trumpet doesn't exist), n >= 20 -> n - 4

_TRUMPET_RANGE = range(16, 20)


def note_block_instrument_924_to_944(value: int) -> int:
    """Remap NoteBlockInstrument from v924 -> v944 (clientbound)."""
    if value >= 16:
        return value + 4
    return value


def note_block_instrument_944_to_924(value: int) -> int | None:
    """Remap NoteBlockInstrument from v944 -> v924 (serverbound).

    Returns None if the value is a trumpet variant (16-19) that doesn't exist in v924.
    """
    if value in _TRUMPET_RANGE:
        return None
    if value >= 20:
        return value - 4
    return value
