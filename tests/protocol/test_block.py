"""Block states carried between versions: the generated tables, the packets that hold a block state
id, and the chunk palettes."""

from __future__ import annotations

import struct

import pytest

from endweave._pipeline import Action, TranslationError, Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

UPDATE_BLOCK = 21
LEVEL_EVENT = 25
FULL_CHUNK_DATA = 58
UPDATE_BLOCK_SYNCED = 110
LEVEL_SOUND_EVENT = 123
CLIENT_CACHE_STATUS = 129
UPDATE_SUB_CHUNK_BLOCKS = 172
SUB_CHUNK = 174

# minecraft:oak_stairs facing 1, right side up: OLD has no corner, NEW adds it.
OAK_STAIRS_OLD = 3302397712
OAK_STAIRS_NEW = 4233957882
OAK_STAIRS_OUTER = 1227992387
# minecraft:white_wool_stairs, which OLD lacks, and the smooth quartz stairs it falls back to.
WOOL_STAIRS = 1832763565
QUARTZ_STAIRS = 2049139513
# minecraft:oak_fence: OLD has no connections, NEW has four.
OAK_FENCE_OLD = 1997655867
OAK_FENCE_NEW = 425458381
OAK_FENCE_JOINED = 480702495
# minecraft:fire age 3: NEWEST adds connections.
FIRE_OLD = 2280018795
FIRE_NEWEST = 3997454090
STONE = 2150698529
AIR = 3690217760
CUSTOM = 0x12345678  # in no vanilla palette, as an add-on block's id is
# minecraft:oak_stairs facing east and north, at OLD and at NEW with each corner.
EAST_STAIRS_OLD = 3240922889
EAST_STAIRS = {"none": 4172483059, "inner_left": 705466535, "outer_left": 1289467210}
NORTH_STAIRS_OLD = 3179448066
NORTH_STAIRS = {"none": 61940232}
UPSIDE_DOWN_NORTH_STAIRS_OLD = 1718578519
# minecraft:oak_fence and minecraft:glass_pane at NEW, by the side they connect on.
OAK_FENCE = {None: OAK_FENCE_NEW, "east": 3720363220, "west": 3665119106}
GLASS_PANE_OLD = 1848427078
GLASS_PANE = {None: 1582595964, "north": 3519003817}
GLASS = 927668178

DESTROY_BLOCK = 2001
START_BLOCK_CRACKING = 3600


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


def signed(runtime_id: int) -> int:
    return struct.unpack("<i", struct.pack("<I", runtime_id))[0]


def string(value: bytes) -> bytes:
    return uvarint(len(value)) + value


def update_block(runtime_id: int) -> bytes:
    return varint(1) + varint(64) + varint(-2) + uvarint(runtime_id) + uvarint(3) + uvarint(0)


def level_event(event: int, data: int) -> bytes:
    return varint(event) + struct.pack("<fff", 1.0, 64.0, -2.0) + varint(signed(data))


def level_sound_event(sound: bytes, data: int) -> bytes:
    position = struct.pack("<fff", 1.0, 64.0, -2.0)
    return string(sound) + position + varint(signed(data)) + string(b"") + bytes(2) + bytes(8) + b"\x00"


def sub_chunk(*palette: int) -> bytes:
    """A version 9 sub-chunk with one storage holding every block at palette index 0."""
    entries = b"".join(varint(signed(i)) for i in palette)
    return bytes([9, 1, 0, (1 << 1) | 1]) + bytes(512) + varint(len(palette)) + entries


def placed_sub_chunk(blocks: dict[tuple[int, int, int], int]) -> bytes:
    """A version 9 sub-chunk with one storage: air, and `blocks` at their x, y, z."""
    palette = [AIR, *dict.fromkeys(blocks.values())]
    bits = next(b for b in (1, 2, 3, 4, 5, 6, 8, 16) if 1 << b >= len(palette))
    per_word = 32 // bits
    words = [0] * -(-4096 // per_word)
    for (x, y, z), runtime_id in blocks.items():
        index = (x << 8) | (z << 4) | y
        words[index // per_word] |= palette.index(runtime_id) << (index % per_word * bits)
    entries = b"".join(varint(signed(i)) for i in palette)
    return bytes([9, 1, 0, (bits << 1) | 1]) + struct.pack(f"<{len(words)}I", *words) + varint(len(palette)) + entries


def read_uvarint(data: bytes, pos: int) -> tuple[int, int]:
    value = shift = 0
    while True:
        byte = data[pos]
        pos += 1
        value |= (byte & 0x7F) << shift
        shift += 7
        if not byte & 0x80:
            return value, pos


def read_sub_chunk(data: bytes) -> dict[tuple[int, int, int], int]:
    """The block at each x, y, z of the first storage of a version 9 sub-chunk."""
    bits, pos = data[3] >> 1, 4
    per_word = 32 // bits
    count = -(-4096 // per_word)
    words = struct.unpack_from(f"<{count}I", data, pos)
    size, pos = read_uvarint(data, pos + count * 4)
    palette = []
    for _ in range(size >> 1):
        value, pos = read_uvarint(data, pos)
        palette.append(((value >> 1) ^ -(value & 1)) & 0xFFFFFFFF)
    blocks = {}
    for x, y, z in ((x, y, z) for x in range(16) for y in range(16) for z in range(16)):
        index = (x << 8) | (z << 4) | y
        blocks[x, y, z] = palette[(words[index // per_word] >> (index % per_word * bits)) & ((1 << bits) - 1)]
    return blocks


def level_chunk(serialized: bytes, count: int = 1, request_limit: int | None = None) -> bytes:
    limit = b"\x00" if request_limit is None else b"\x01" + varint(request_limit)
    return varint(2) + varint(3) + varint(0) + uvarint(count) + limit + b"\x00" + uvarint(0) + string(serialized)


def sub_chunk_entry(serialized: bytes, offset: tuple[int, int, int] = (-1, 0, 1)) -> bytes:
    return struct.pack("<bbb", *offset) + b"\x01\x01" + string(serialized) + bytes(4) + b"\x00"


def sub_chunk_packet(*entries: bytes) -> bytes:
    return b"\x00" + varint(0) + struct.pack("<iii", 1, 2, 3) + uvarint(len(entries)) + b"".join(entries)


def read_sub_chunk_entries(payload: bytes) -> list[bytes]:
    count, pos = read_uvarint(payload, 14)
    entries = []
    for _ in range(count):
        size, pos = read_uvarint(payload, pos + 5)
        entries.append(payload[pos : pos + size])
        pos += size + 5
    return entries


def sub_chunk_block(runtime_id: int) -> bytes:
    return varint(1) + varint(2) + varint(3) + uvarint(runtime_id) + uvarint(3) + uvarint(0) + uvarint(0)


class TestActions:
    @pytest.mark.parametrize(("source", "target"), [(OLD, NEW), (NEW, OLD), (NEW, NEWEST), (NEWEST, OLD)])
    def test_translates_every_packet_that_holds_a_block_state(self, source: int, target: int) -> None:
        actions = Translator(source, target).actions

        for packet_id in (
            UPDATE_BLOCK,
            LEVEL_EVENT,
            FULL_CHUNK_DATA,
            UPDATE_BLOCK_SYNCED,
            LEVEL_SOUND_EVENT,
            CLIENT_CACHE_STATUS,
            UPDATE_SUB_CHUNK_BLOCKS,
            SUB_CHUNK,
        ):
            assert actions[packet_id] is Action.TRANSLATE


class TestBlockIds:
    def test_gives_an_older_state_the_properties_the_newer_version_added(self) -> None:
        assert Translator(OLD, NEW).translate(UPDATE_BLOCK, update_block(OAK_STAIRS_OLD)) == update_block(
            OAK_STAIRS_NEW
        )
        assert Translator(OLD, NEW).translate(UPDATE_BLOCK, update_block(OAK_FENCE_OLD)) == update_block(OAK_FENCE_NEW)

    def test_takes_away_what_the_older_version_lacks(self) -> None:
        translator = Translator(NEW, OLD)

        assert translator.translate(UPDATE_BLOCK, update_block(OAK_STAIRS_OUTER)) == update_block(OAK_STAIRS_OLD)
        assert translator.translate(UPDATE_BLOCK, update_block(OAK_FENCE_JOINED)) == update_block(OAK_FENCE_OLD)

    def test_shows_an_older_client_the_fallback_for_a_block_it_lacks(self) -> None:
        assert Translator(NEW, OLD).translate(UPDATE_BLOCK, update_block(WOOL_STAIRS)) == update_block(QUARTZ_STAIRS)

    def test_carries_a_state_across_every_hop_between_the_ends(self) -> None:
        assert Translator(NEWEST, OLD).translate(UPDATE_BLOCK, update_block(FIRE_NEWEST)) == update_block(FIRE_OLD)
        assert Translator(OLD, NEWEST).translate(UPDATE_BLOCK, update_block(OAK_STAIRS_OLD)) == update_block(
            OAK_STAIRS_NEW
        )

    def test_leaves_a_state_both_versions_share(self) -> None:
        assert Translator(NEW, OLD).translate(UPDATE_BLOCK, update_block(STONE)) == update_block(STONE)

    def test_leaves_a_state_no_vanilla_palette_has(self) -> None:
        assert Translator(OLD, NEW).translate(UPDATE_BLOCK, update_block(CUSTOM)) == update_block(CUSTOM)

    def test_reaches_ids_inside_nested_lists(self) -> None:
        def packet(runtime_id: int) -> bytes:
            return varint(0) + varint(0) + varint(0) + uvarint(1) + sub_chunk_block(runtime_id) + uvarint(0)

        assert Translator(OLD, NEW).translate(UPDATE_SUB_CHUNK_BLOCKS, packet(OAK_FENCE_OLD)) == packet(OAK_FENCE_NEW)


class TestLevelEvent:
    def test_carries_the_block_a_destroy_particle_shows(self) -> None:
        assert Translator(OLD, NEW).translate(LEVEL_EVENT, level_event(DESTROY_BLOCK, OAK_STAIRS_OLD)) == level_event(
            DESTROY_BLOCK, OAK_STAIRS_NEW
        )

    def test_leaves_data_that_is_not_a_block(self) -> None:
        payload = level_event(START_BLOCK_CRACKING, OAK_STAIRS_OLD)

        assert Translator(OLD, NEW).translate(LEVEL_EVENT, payload) == payload


class TestLevelSoundEvent:
    def test_carries_the_block_a_placement_sounds_for(self) -> None:
        assert Translator(NEW, OLD).translate(
            LEVEL_SOUND_EVENT, level_sound_event(b"place", OAK_FENCE_JOINED)
        ) == level_sound_event(b"place", OAK_FENCE_OLD)

    def test_leaves_a_sound_whose_data_is_not_a_block(self) -> None:
        payload = level_sound_event(b"hurt", OAK_FENCE_JOINED)

        assert Translator(NEW, OLD).translate(LEVEL_SOUND_EVENT, payload) == payload


class TestClientCache:
    @pytest.mark.parametrize(("source", "target"), [(OLD, NEW), (NEW, OLD)])
    def test_turns_the_blob_cache_off(self, source: int, target: int) -> None:
        assert Translator(source, target).translate(CLIENT_CACHE_STATUS, b"\x01") == b"\x00"


class TestChunkPalettes:
    def test_rewrites_a_full_chunk_palette_and_keeps_what_follows(self) -> None:
        payload = level_chunk(sub_chunk(STONE, OAK_STAIRS_OLD) + b"\xaa\xbb")

        assert Translator(OLD, NEW).translate(FULL_CHUNK_DATA, payload) == level_chunk(
            sub_chunk(STONE, OAK_STAIRS_NEW) + b"\xaa\xbb"
        )

    def test_leaves_a_chunk_whose_sub_chunks_are_requested(self) -> None:
        payload = level_chunk(b"\xaa", count=0, request_limit=-1)

        assert Translator(OLD, NEW).translate(FULL_CHUNK_DATA, payload) == payload

    def test_rewrites_a_requested_sub_chunk(self) -> None:
        payload = sub_chunk_packet(sub_chunk_entry(sub_chunk(OAK_FENCE_JOINED, WOOL_STAIRS)))

        assert Translator(NEW, OLD).translate(SUB_CHUNK, payload) == sub_chunk_packet(
            sub_chunk_entry(sub_chunk(OAK_FENCE_OLD, QUARTZ_STAIRS))
        )

    def test_refuses_a_sub_chunk_it_cannot_read(self) -> None:
        with pytest.raises(TranslationError):
            Translator(OLD, NEW).translate(FULL_CHUNK_DATA, level_chunk(bytes([7, 1, 0])))


def translate_chunk(
    blocks: dict[tuple[int, int, int], int], source: int = OLD, target: int = NEW
) -> dict[tuple[int, int, int], int]:
    payload = Translator(source, target).translate(FULL_CHUNK_DATA, level_chunk(placed_sub_chunk(blocks)))
    size, pos = read_uvarint(payload, len(level_chunk(b"")) - 1)
    return read_sub_chunk(payload[pos : pos + size])


class TestStairCorners:
    def test_turns_a_stair_behind_into_an_outer_corner(self) -> None:
        blocks = translate_chunk({(5, 0, 5): EAST_STAIRS_OLD, (6, 0, 5): NORTH_STAIRS_OLD})

        assert blocks[5, 0, 5] == EAST_STAIRS["outer_left"]
        assert blocks[6, 0, 5] == NORTH_STAIRS["none"]
        assert blocks[0, 0, 0] == AIR

    def test_turns_a_stair_in_front_into_an_inner_corner(self) -> None:
        blocks = translate_chunk({(5, 0, 5): EAST_STAIRS_OLD, (4, 0, 5): NORTH_STAIRS_OLD})

        assert blocks[5, 0, 5] == EAST_STAIRS["inner_left"]

    def test_keeps_a_run_of_the_same_stair_straight(self) -> None:
        blocks = translate_chunk({(5, 0, 5): EAST_STAIRS_OLD, (6, 0, 5): NORTH_STAIRS_OLD, (5, 0, 6): EAST_STAIRS_OLD})

        assert blocks[5, 0, 5] == EAST_STAIRS["none"]

    def test_ignores_a_stair_on_the_other_half(self) -> None:
        blocks = translate_chunk({(5, 0, 5): EAST_STAIRS_OLD, (6, 0, 5): UPSIDE_DOWN_NORTH_STAIRS_OLD})

        assert blocks[5, 0, 5] == EAST_STAIRS["none"]

    def test_keeps_the_corner_through_a_newer_hop(self) -> None:
        blocks = translate_chunk({(5, 0, 5): EAST_STAIRS_OLD, (6, 0, 5): NORTH_STAIRS_OLD}, target=NEWEST)

        assert blocks[5, 0, 5] == EAST_STAIRS["outer_left"]

    def test_reads_neighbours_from_other_requested_sub_chunks(self) -> None:
        payload = sub_chunk_packet(
            sub_chunk_entry(placed_sub_chunk({(15, 0, 5): EAST_STAIRS_OLD}), (0, 0, 0)),
            sub_chunk_entry(placed_sub_chunk({(0, 0, 5): NORTH_STAIRS_OLD}), (1, 0, 0)),
        )

        here, east = read_sub_chunk_entries(Translator(OLD, NEW).translate(SUB_CHUNK, payload))

        assert read_sub_chunk(here)[15, 0, 5] == EAST_STAIRS["outer_left"]
        assert read_sub_chunk(east)[0, 0, 5] == NORTH_STAIRS["none"]

    def test_leaves_the_corners_a_newer_server_sent(self) -> None:
        blocks = translate_chunk({(5, 0, 5): EAST_STAIRS["outer_left"]}, NEW, NEWEST)

        assert blocks[5, 0, 5] == EAST_STAIRS["outer_left"]


class TestFenceConnections:
    def test_joins_a_fence_to_a_full_block(self) -> None:
        blocks = translate_chunk({(5, 0, 5): OAK_FENCE_OLD, (6, 0, 5): STONE})

        assert blocks[5, 0, 5] == OAK_FENCE["east"]

    def test_joins_two_fences(self) -> None:
        blocks = translate_chunk({(5, 0, 5): OAK_FENCE_OLD, (6, 0, 5): OAK_FENCE_OLD})

        assert blocks[5, 0, 5] == OAK_FENCE["east"]
        assert blocks[6, 0, 5] == OAK_FENCE["west"]

    def test_keeps_a_fence_and_a_pane_apart(self) -> None:
        blocks = translate_chunk({(5, 0, 5): OAK_FENCE_OLD, (6, 0, 5): GLASS_PANE_OLD})

        assert blocks[5, 0, 5] == OAK_FENCE[None]
        assert blocks[6, 0, 5] == GLASS_PANE[None]

    def test_joins_a_pane_to_glass(self) -> None:
        blocks = translate_chunk({(5, 0, 5): GLASS_PANE_OLD, (5, 0, 4): GLASS})

        assert blocks[5, 0, 5] == GLASS_PANE["north"]

    def test_joins_a_fence_only_to_the_back_of_a_stair(self) -> None:
        blocks = translate_chunk({(5, 0, 5): EAST_STAIRS_OLD, (6, 0, 5): OAK_FENCE_OLD, (4, 0, 5): OAK_FENCE_OLD})

        assert blocks[6, 0, 5] == OAK_FENCE["west"]
        assert blocks[4, 0, 5] == OAK_FENCE[None]
