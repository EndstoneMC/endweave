"""Biomes a client's version lacks: left out of the biome definitions and given a fallback in chunks."""

from __future__ import annotations

import struct

import pytest

from endweave._pipeline import Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

BIOME_DEFINITION_LIST = 122

PLAINS = 1
ICE_CAVES = 196  # NEWEST has it, NEW and OLD lack it


def uvarint(value: int) -> bytes:
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        out.append(byte | (0x80 if value else 0))
        if not value:
            return bytes(out)


def string(value: str) -> bytes:
    return uvarint(len(value)) + value.encode()


def optional(value: bytes | None) -> bytes:
    return b"\x00" if value is None else b"\x01" + value


def definition(biome_id: int, tags: list[int] | None = None) -> bytes:
    """A biome definition with no chunk generation data."""
    tag_data = None if tags is None else uvarint(len(tags)) + b"".join(struct.pack("<H", tag) for tag in tags)
    return (
        struct.pack("<H", biome_id)
        + struct.pack("<5f", 0.5, 0.5, 0.0, 0.1, 0.2)
        + struct.pack("<i", -1)
        + b"\x01"
        + optional(tag_data)
        + b"\x00"
    )


def biome_definition_list(definitions: dict[int, bytes], strings: list[str]) -> bytes:
    """Definitions by the index of their name in `strings`."""
    entries = b"".join(struct.pack("<H", index) + data for index, data in sorted(definitions.items()))
    return uvarint(len(definitions)) + entries + uvarint(len(strings)) + b"".join(string(s) for s in strings)


class TestBiomeDefinitions:
    STRINGS = ["minecraft:plains", "minecraft:ice_caves", "monster", "minecraft:desert"]
    PLAINS_DEFINITION = definition(PLAINS, tags=[2])
    DESERT_DEFINITION = definition(2, tags=[2])

    def payload(self) -> bytes:
        return biome_definition_list(
            {0: self.PLAINS_DEFINITION, 1: definition(ICE_CAVES, tags=[2]), 3: self.DESERT_DEFINITION}, self.STRINGS
        )

    @pytest.mark.parametrize("target", [NEW, OLD])
    def test_leaves_out_a_biome_the_client_lacks_and_keeps_every_index(self, target: int) -> None:
        assert Translator(NEWEST, target).translate(BIOME_DEFINITION_LIST, self.payload()) == biome_definition_list(
            {0: self.PLAINS_DEFINITION, 3: self.DESERT_DEFINITION}, self.STRINGS
        )

    def test_keeps_every_biome_a_newer_client_has(self) -> None:
        payload = biome_definition_list({0: self.PLAINS_DEFINITION, 3: self.DESERT_DEFINITION}, self.STRINGS)

        assert Translator(NEW, NEWEST).translate(BIOME_DEFINITION_LIST, payload) == payload
