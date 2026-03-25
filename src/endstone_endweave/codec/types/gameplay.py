"""Gameplay-related compound types shared across protocol handlers."""

from dataclasses import dataclass
from typing import Any

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.types.primitives import (
    BOOL,
    FLOAT_LE,
    INT_LE,
    STRING,
    UINT_LE,
    UVAR_INT,
    VAR_INT,
    Type,
)
from endstone_endweave.codec.writer import PacketWriter

# ---------------------------------------------------------------------------
# Game rules (conditional union per entry)
# ---------------------------------------------------------------------------


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
            rule_value: bool | int | float
            if type_id == 1:
                rule_value = BOOL.read(reader)
            elif type_id == 2:
                rule_value = VAR_INT.read(reader)
            elif type_id == 3:
                rule_value = FLOAT_LE.read(reader)
            else:
                raise ValueError(f"Unknown game rule type: {type_id}")
            rules.append(GameRule(name=name, editable=editable, type_id=type_id, value=rule_value))
        return rules

    def write(self, writer: PacketWriter, value: list[GameRule]) -> None:
        UVAR_INT.write(writer, len(value))
        for rule in value:
            STRING.write(writer, rule.name)
            BOOL.write(writer, rule.editable)
            UVAR_INT.write(writer, rule.type_id)
            if rule.type_id == 1:
                BOOL.write(writer, rule.value)
            elif rule.type_id == 2:
                VAR_INT.write(writer, rule.value)
            elif rule.type_id == 3:
                FLOAT_LE.write(writer, rule.value)
            else:
                raise ValueError(f"Unknown game rule type: {rule.type_id}")


# ---------------------------------------------------------------------------
# Experiments (count + entries + optional ever_toggled)
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
    """UINT_LE count + (STRING + BOOL)[] entries. Used in v924+."""

    def read(self, reader: PacketReader) -> list[Experiment]:
        count = UINT_LE.read(reader)
        return [Experiment(name=STRING.read(reader), enabled=BOOL.read(reader)) for _ in range(count)]

    def write(self, writer: PacketWriter, value: list[Experiment]) -> None:
        UINT_LE.write(writer, len(value))
        for exp in value:
            STRING.write(writer, exp.name)
            BOOL.write(writer, exp.enabled)


class _ExperimentsV860Type(Type[list[Experiment]]):
    """INT_LE count + (STRING + BOOL)[] entries. Used in v860/v898."""

    def read(self, reader: PacketReader) -> list[Experiment]:
        count = INT_LE.read(reader)
        return [Experiment(name=STRING.read(reader), enabled=BOOL.read(reader)) for _ in range(count)]

    def write(self, writer: PacketWriter, value: list[Experiment]) -> None:
        INT_LE.write(writer, len(value))
        for exp in value:
            STRING.write(writer, exp.name)
            BOOL.write(writer, exp.enabled)


# ---------------------------------------------------------------------------
# Singletons
# ---------------------------------------------------------------------------

GAME_RULES = _GameRulesType()
EXPERIMENTS = _ExperimentsType()
EXPERIMENTS_V860 = _ExperimentsV860Type()
