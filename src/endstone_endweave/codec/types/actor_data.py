"""ActorData (entity metadata) compound type.

See Also:
    com.viaversion.viaversion.api.type.types.entitydata.EntityDataListType
"""

from dataclasses import dataclass
from typing import Any

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.types.nbt import NAMED_COMPOUND_TAG
from endstone_endweave.codec.types.primitives import (
    BLOCK_POS,
    BYTE,
    FLOAT_LE,
    SHORT_LE,
    STRING,
    UVAR_INT,
    VAR_INT,
    VAR_INT64,
    VEC3,
    Type,
)
from endstone_endweave.codec.writer import PacketWriter

# ActorData value type IDs -> codec type
_VALUE_TYPES: dict[int, Type[Any]] = {
    0: BYTE,  # Byte
    1: SHORT_LE,  # Short
    2: VAR_INT,  # Int
    3: FLOAT_LE,  # Float
    4: STRING,  # String
    5: NAMED_COMPOUND_TAG,  # CompoundTag
    6: BLOCK_POS,  # BlockPos
    7: VAR_INT64,  # Int64
    8: VEC3,  # Vec3
}


@dataclass
class ActorDataEntry:
    """A single ActorData (entity metadata) entry.

    Attributes:
        key: Metadata key ID.
        type_id: Value type ID (0=Byte, 1=Short, 2=Int, 3=Float,
            4=String, 5=CompoundTag, 6=BlockPos, 7=Int64, 8=Vec3).
        value: The metadata value, typed according to type_id.
    """

    key: int
    type_id: int
    value: Any


class _ActorDataListType(Type[list["ActorDataEntry"]]):
    """ActorData list: uvarint count + entries of (key, type_id, value)."""

    def read(self, reader: PacketReader) -> list[ActorDataEntry]:
        count = UVAR_INT.read(reader)
        entries: list[ActorDataEntry] = []
        for _ in range(count):
            key = UVAR_INT.read(reader)
            type_id = UVAR_INT.read(reader)
            value = _VALUE_TYPES[type_id].read(reader)
            entries.append(ActorDataEntry(key, type_id, value))
        return entries

    def write(self, writer: PacketWriter, value: list[ActorDataEntry]) -> None:
        UVAR_INT.write(writer, len(value))
        for entry in value:
            UVAR_INT.write(writer, entry.key)
            UVAR_INT.write(writer, entry.type_id)
            _VALUE_TYPES[entry.type_id].write(writer, entry.value)


ACTOR_DATA_LIST = _ActorDataListType()
