"""Gameplay-related compound types shared across protocol handlers."""

from dataclasses import dataclass
from typing import Any

from ..reader import PacketReader
from ..writer import PacketWriter
from .primitives import (
    BOOL,
    FLOAT_LE,
    INT_LE,
    STRING,
    UINT_LE,
    UVAR_INT,
    VAR_INT,
    Type,
)

# ---------------------------------------------------------------------------
# Game rules (conditional union per entry)
# ---------------------------------------------------------------------------

_RULE_VALUE_TYPES: dict[int, Type[Any]] = {
    1: BOOL,
    2: VAR_INT,
    3: FLOAT_LE,
}


@dataclass
class GameRule:
    """A game rule entry with typed value.

    Attributes:
        name: Rule name string.
        editable: Whether the rule can be modified.
        type_id: Rule type (1=bool, 2=varint, 3=float).
        value: Rule value (type depends on type_id).
    """

    name: str
    editable: bool
    type_id: int
    value: Any


class _GameRulesType(Type[list["GameRule"]]):
    """UVAR_INT count + per-rule entries with conditional value type."""

    def read(self, reader: PacketReader) -> list[GameRule]:
        count = UVAR_INT.read(reader)
        rules: list[GameRule] = []
        for _ in range(count):
            name = STRING.read(reader)
            editable = BOOL.read(reader)
            type_id = UVAR_INT.read(reader)
            value_type = _RULE_VALUE_TYPES.get(type_id)
            if value_type is None:
                raise ValueError(f"Unknown game rule type: {type_id}")
            rules.append(GameRule(name=name, editable=editable, type_id=type_id, value=value_type.read(reader)))
        return rules

    def write(self, writer: PacketWriter, value: list[GameRule]) -> None:
        UVAR_INT.write(writer, len(value))
        for rule in value:
            STRING.write(writer, rule.name)
            BOOL.write(writer, rule.editable)
            UVAR_INT.write(writer, rule.type_id)
            value_type = _RULE_VALUE_TYPES.get(rule.type_id)
            if value_type is None:
                raise ValueError(f"Unknown game rule type: {rule.type_id}")
            value_type.write(writer, rule.value)


# ---------------------------------------------------------------------------
# Experiments (count + entries)
# ---------------------------------------------------------------------------


@dataclass
class Experiment:
    """An experiment entry (name + enabled flag).

    Attributes:
        name: Experiment name string.
        enabled: Whether the experiment is enabled.
    """

    name: str
    enabled: bool


class _ExperimentsType(Type[list[Experiment]]):
    """Count-prefixed list of (STRING + BOOL) experiment entries.

    Args:
        count_type: Type used for the list count (UINT_LE for v924+, INT_LE for v860/v898).
    """

    def __init__(self, count_type: Type[int] = UINT_LE) -> None:
        self._count_type = count_type

    def read(self, reader: PacketReader) -> list[Experiment]:
        count = self._count_type.read(reader)
        return [Experiment(name=STRING.read(reader), enabled=BOOL.read(reader)) for _ in range(count)]

    def write(self, writer: PacketWriter, value: list[Experiment]) -> None:
        self._count_type.write(writer, len(value))
        for exp in value:
            STRING.write(writer, exp.name)
            BOOL.write(writer, exp.enabled)


# ---------------------------------------------------------------------------
# Singletons
# ---------------------------------------------------------------------------

GAME_RULES = _GameRulesType()
EXPERIMENTS = _ExperimentsType(UINT_LE)
EXPERIMENTS_V860 = _ExperimentsType(INT_LE)
