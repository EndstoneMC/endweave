"""Biomes a client's version lacks: left out of the biome definitions and given a fallback in chunks."""

from __future__ import annotations

import struct

import pytest

from endweave._pipeline import TranslationError, Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

FULL_CHUNK_DATA = 58
BIOME_DEFINITION_LIST = 122

PLAINS = 1
DRIPSTONE_CAVES = 188
ICE_CAVES = 196  # NEWEST has it, NEW and OLD lack it

STONE = 2150698529
# minecraft:oak_stairs facing 1, right side up: OLD has no corner, NEW adds it.
OAK_STAIRS_OLD = 3302397712
OAK_STAIRS_NEW = 4233957882

SAME_AS_BELOW = b"\xff"


def uvarint(value: int) -> bytes:
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        out.append(byte | (0x80 if value else 0))
        if not value:
            return bytes(out)


def varint(value: int) -> bytes:
    return uvarint(((value << 1) ^ (value >> 31)) & 0xFFFFFFFF)


def string(value: str) -> bytes:
    return uvarint(len(value)) + value.encode()


def optional(value: bytes | None) -> bytes:
    return b"\x00" if value is None else b"\x01" + value


def uint32s(values: list[int]) -> bytes:
    return uvarint(len(values)) + b"".join(struct.pack("<I", value) for value in values)


def capped_surface(
    floor: list[int], ceiling: list[int], sea: int | None, foundation: int | None, beach: int | None
) -> bytes:
    blocks = (None if block is None else struct.pack("<I", block) for block in (sea, foundation, beach))
    return uint32s(floor) + uint32s(ceiling) + b"".join(optional(block) for block in blocks)


def noise_gradient_surface(non_replaceable: list[int]) -> bytes:
    """No gradient ranges, and a noise with no amplitudes."""
    return uint32s(non_replaceable) + uvarint(0) + string("") + struct.pack("<i", 0) + uvarint(0)


def surface_builder(capped: bytes | None = None, noise: bytes | None = None) -> bytes:
    """No surface materials or mesa, and none of the four built-in surfaces."""
    return b"\x00" + bytes(4) + b"\x00" + optional(capped) + optional(noise)


def definition(biome_id: int, tags: list[int] | None = None, surface: bytes | None = None) -> bytes:
    """A biome definition whose chunk generation data, if any, is its surface builder and nothing else."""
    tag_data = None if tags is None else uvarint(len(tags)) + b"".join(struct.pack("<H", tag) for tag in tags)
    chunk_gen = None if surface is None else bytes(9) + b"\x01" + surface + b"\x00"
    return (
        struct.pack("<H", biome_id)
        + struct.pack("<5f", 0.5, 0.5, 0.0, 0.1, 0.2)
        + struct.pack("<i", -1)
        + b"\x01"
        + optional(tag_data)
        + optional(chunk_gen)
    )


def biome_definition_list(definitions: dict[int, bytes], strings: list[str]) -> bytes:
    """Definitions by the index of their name in `strings`."""
    entries = b"".join(struct.pack("<H", index) + data for index, data in sorted(definitions.items()))
    return uvarint(len(definitions)) + entries + uvarint(len(strings)) + b"".join(string(s) for s in strings)


def biomes(*palette: int) -> bytes:
    """A biome storage with every block of the sub-chunk at palette index 0."""
    if len(palette) == 1:
        return b"\x01" + varint(palette[0])
    return bytes([(1 << 1) | 1]) + bytes(512) + varint(len(palette)) + b"".join(varint(i) for i in palette)


def sub_chunk(runtime_id: int) -> bytes:
    """A version 9 sub-chunk holding one block everywhere."""
    signed = struct.unpack("<i", struct.pack("<I", runtime_id))[0]
    return bytes([9, 1, 0, 1]) + varint(signed)


def level_chunk(
    serialized: bytes,
    count: int = 1,
    request_limit: int | None = None,
    dimension: int = 0,
    biome_update: bool | None = None,
) -> bytes:
    """A LevelChunkPacket; `biome_update` is NEWEST's flag, which the older versions lack."""
    limit = b"\x00" if request_limit is None else b"\x01" + varint(request_limit)
    flag = b"" if biome_update is None else bytes([biome_update])
    return (
        varint(2)
        + varint(3)
        + varint(dimension)
        + uvarint(count)
        + limit
        + b"\x00"
        + uvarint(0)
        + uvarint(len(serialized))
        + serialized
        + flag
    )


def column(*storages: bytes, height: int = 24) -> bytes:
    """`storages` from the bottom of a dimension `height` sub-chunks high, the rest the same as below."""
    return b"".join(storages) + SAME_AS_BELOW * (height - len(storages))


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


class TestBiomeDefinitionBlocks:
    def test_rewrites_the_blocks_a_capped_surface_names(self) -> None:
        def payload(stairs: int) -> bytes:
            capped = capped_surface([stairs, STONE], [stairs], stairs, None, STONE)
            return biome_definition_list({0: definition(PLAINS, surface=surface_builder(capped=capped))}, ["plains"])

        assert Translator(OLD, NEW).translate(BIOME_DEFINITION_LIST, payload(OAK_STAIRS_OLD)) == payload(OAK_STAIRS_NEW)

    def test_rewrites_the_blocks_a_noise_gradient_surface_keeps(self) -> None:
        def payload(stairs: int) -> bytes:
            noise = noise_gradient_surface([STONE, stairs])
            return biome_definition_list({0: definition(PLAINS, surface=surface_builder(noise=noise))}, ["plains"])

        assert Translator(OLD, NEW).translate(BIOME_DEFINITION_LIST, payload(OAK_STAIRS_OLD)) == payload(OAK_STAIRS_NEW)


class TestChunkBiomes:
    def test_gives_a_biome_the_client_lacks_its_fallback(self) -> None:
        def payload(biome: int) -> bytes:
            return sub_chunk(STONE) + column(biomes(PLAINS, biome), SAME_AS_BELOW, biomes(biome)) + b"\x00"

        source = level_chunk(payload(ICE_CAVES), biome_update=False)

        assert Translator(NEWEST, NEW).translate(FULL_CHUNK_DATA, source) == level_chunk(payload(DRIPSTONE_CAVES))

    def test_rewrites_the_biomes_of_a_chunk_whose_sub_chunks_are_requested(self) -> None:
        def payload(biome: int) -> bytes:
            return column(biomes(biome)) + b"\x00"

        source = level_chunk(payload(ICE_CAVES), count=0, request_limit=-1, biome_update=False)

        assert Translator(NEWEST, OLD).translate(FULL_CHUNK_DATA, source) == level_chunk(
            payload(DRIPSTONE_CAVES), count=0, request_limit=-1
        )

    def test_reads_as_many_storages_as_the_dimension_has_sub_chunks(self) -> None:
        def payload(biome: int) -> bytes:
            return sub_chunk(STONE) + column(biomes(biome), height=8) + b"\x00"

        source = level_chunk(payload(ICE_CAVES), dimension=1, biome_update=False)

        assert Translator(NEWEST, NEW).translate(FULL_CHUNK_DATA, source) == level_chunk(
            payload(DRIPSTONE_CAVES), dimension=1
        )

    def test_leaves_the_biomes_of_a_dimension_it_has_no_definition_for(self) -> None:
        payload = sub_chunk(STONE) + b"\xaa\xbb"

        assert Translator(NEWEST, NEW).translate(
            FULL_CHUNK_DATA, level_chunk(payload, dimension=3, biome_update=False)
        ) == level_chunk(payload, dimension=3)

    def test_drops_a_biome_update(self) -> None:
        source = level_chunk(column(biomes(ICE_CAVES)), count=0, biome_update=True)

        assert Translator(NEWEST, NEW).translate(FULL_CHUNK_DATA, source) is None

    def test_refuses_biomes_it_cannot_read(self) -> None:
        with pytest.raises(TranslationError):
            Translator(NEWEST, NEW).translate(
                FULL_CHUNK_DATA, level_chunk(sub_chunk(STONE) + biomes(ICE_CAVES), biome_update=False)
            )
