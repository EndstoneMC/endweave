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
CUSTOM = 0x12345678  # in no vanilla palette, as an add-on block's id is

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


def level_chunk(serialized: bytes, count: int = 1, request_limit: int | None = None) -> bytes:
    limit = b"\x00" if request_limit is None else b"\x01" + varint(request_limit)
    return varint(2) + varint(3) + varint(0) + uvarint(count) + limit + b"\x00" + uvarint(0) + string(serialized)


def sub_chunk_packet(serialized: bytes) -> bytes:
    entry = bytes([0xFF, 0x00, 0x01, 0x01]) + b"\x01" + string(serialized) + bytes(4) + b"\x00"
    return b"\x00" + varint(0) + struct.pack("<iii", 1, 2, 3) + uvarint(1) + entry


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
        payload = sub_chunk_packet(sub_chunk(OAK_FENCE_JOINED, WOOL_STAIRS))

        assert Translator(NEW, OLD).translate(SUB_CHUNK, payload) == sub_chunk_packet(
            sub_chunk(OAK_FENCE_OLD, QUARTZ_STAIRS)
        )

    def test_refuses_a_sub_chunk_it_cannot_read(self) -> None:
        with pytest.raises(TranslationError):
            Translator(OLD, NEW).translate(FULL_CHUNK_DATA, level_chunk(bytes([7, 1, 0])))
