"""Declarative ID mapping data for version-pair protocol translation.

Each version pair's ``protocol.py`` instantiates a :class:`MappingData` with
the ID shifts for that step.  The Python module itself serves as the data
file, keeping mapping declarations and handler registration self-contained.

See Also:
    com.viaversion.viaversion.api.data.MappingData
    com.viaversion.viaversion.api.data.MappingDataBase
"""

from dataclasses import dataclass, field


@dataclass(frozen=True)
class IdShift:
    """Describes new IDs inserted at a position, displacing subsequent IDs.

    Attributes:
        insert_at: First ID of the inserted range.
        count: Number of IDs inserted.
    """

    insert_at: int
    count: int

    def shift_up(self, v: int) -> int:
        """Upgrade remap: shift IDs >= *insert_at* up by *count*.

        Args:
            v: Original ID.

        Returns:
            Remapped ID.
        """
        if v >= self.insert_at:
            return v + self.count
        return v

    def shift_down(self, v: int) -> int:
        """Downgrade remap: collapse the inserted range, shift old IDs back.

        IDs in ``[insert_at, insert_at + count)`` map to *insert_at*
        (the target version's ``Undefined``).  IDs above the range shift
        down by *count*.

        Args:
            v: Original ID.

        Returns:
            Remapped ID.
        """
        if v >= self.insert_at + self.count:
            return v - self.count
        if v >= self.insert_at:
            return self.insert_at
        return v

    def cap(self, v: int) -> int:
        """Lossy downgrade: everything >= *insert_at* becomes *insert_at*.

        Use when the target version cannot meaningfully represent any of
        the newer IDs (not just the inserted range).

        Args:
            v: Original ID.

        Returns:
            Remapped ID.
        """
        if v >= self.insert_at:
            return self.insert_at
        return v


@dataclass(frozen=True)
class MappingData:
    """Declarative ID mapping data for a single version step.

    Attributes:
        sound: LevelSoundEvent ID shift.
        actor_data_sound_key: ActorDataID that holds a sound event value
            (remapped with the same function as *sound*).
        actor_event: ActorEvent ID shift (None if unchanged).
        note_instrument: NoteBlockInstrument ID shift (None if unchanged).
        dropped_actor_data_keys: ActorData keys to filter out entirely.

    See Also:
        com.viaversion.viaversion.api.data.MappingData
    """

    sound: IdShift
    actor_data_sound_key: int
    actor_event: IdShift | None = None
    note_instrument: IdShift | None = None
    dropped_actor_data_keys: frozenset[int] = field(default=frozenset())
