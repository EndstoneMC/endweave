"""Data store compound types (recursive ChangeValue)."""

from dataclasses import dataclass, field
from typing import Any

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.types.primitives import (
    BOOL,
    INT64_LE,
    INT_LE,
    STRING,
    UVAR_INT,
    Type,
)
from endstone_endweave.codec.writer import PacketWriter


@dataclass
class ChangeValue:
    """A recursive data store change value.

    Attributes:
        type_id: Value type (0=empty, 1=bool, 2=int64, 4=string, 6=map).
        value: The value (type depends on type_id).
        entries: Child entries for map type (type_id=6).
    """

    type_id: int
    value: Any = None
    entries: list[tuple[str, "ChangeValue"]] = field(default_factory=list)


class _ChangeValueType(Type[ChangeValue]):
    """Recursive conditional type for data store change values."""

    def read(self, reader: PacketReader) -> ChangeValue:
        type_id = INT_LE.read(reader)
        if type_id == 0:
            return ChangeValue(type_id=0)
        if type_id == 1:
            return ChangeValue(type_id=1, value=BOOL.read(reader))
        if type_id == 2:
            return ChangeValue(type_id=2, value=INT64_LE.read(reader))
        if type_id == 4:
            return ChangeValue(type_id=4, value=STRING.read(reader))
        if type_id == 6:
            count = UVAR_INT.read(reader)
            entries: list[tuple[str, ChangeValue]] = []
            for _ in range(count):
                key = STRING.read(reader)
                child = self.read(reader)
                entries.append((key, child))
            return ChangeValue(type_id=6, entries=entries)
        raise ValueError(f"Invalid data store change data type: {type_id}")

    def write(self, writer: PacketWriter, value: ChangeValue) -> None:
        INT_LE.write(writer, value.type_id)
        if value.type_id == 0:
            return
        if value.type_id == 1:
            BOOL.write(writer, value.value)
            return
        if value.type_id == 2:
            INT64_LE.write(writer, value.value)
            return
        if value.type_id == 4:
            STRING.write(writer, value.value)
            return
        if value.type_id == 6:
            UVAR_INT.write(writer, len(value.entries))
            for key, child in value.entries:
                STRING.write(writer, key)
                self.write(writer, child)
            return
        raise ValueError(f"Invalid data store change data type: {value.type_id}")


CHANGE_VALUE = _ChangeValueType()
