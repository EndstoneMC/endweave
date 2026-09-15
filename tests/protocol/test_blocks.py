"""Wire-level regression cases from the pinned Cloudburst 2168 and 2193 palettes."""

import struct

import pytest

from endweave._pipeline import Action, TranslationError, Translator

# Old/new hashes of the same existing block, including high-bit signed palette values.
BLOCKS = [
    (3240922889, 4172483059),  # oak stairs, bottom, east, straight
    (3820945975, 3265450305),  # cobblestone stairs
    (1997655867, 425458381),  # oak fence
    (1848427078, 1582595964),  # glass pane
    (2449968093, 513174327),  # iron bars
    (3501576853, 1519406791),  # tripwire
]


def uvarint(value: int) -> bytes:
    result = bytearray()
    while value >= 128:
        result.append((value & 127) | 128)
        value >>= 7
    result.append(value)
    return bytes(result)


def varint(value: int) -> bytes:
    if value > 0x7FFFFFFF:
        value -= 0x100000000
    return uvarint(((value << 1) ^ (value >> 31)) & 0xFFFFFFFF)


def string(value: bytes) -> bytes:
    return uvarint(len(value)) + value


def update(runtime_id: int) -> bytes:
    return b"\x00\x00\x00" + uvarint(runtime_id) + b"\x03\x00"


def level_chunk(data: bytes, count: int = 1) -> bytes:
    return b"\x00\x00\x00" + uvarint(count) + b"\x00\x00\x00" + string(data)


def subchunk_packet(data: bytes) -> bytes:
    # Uncached, dimension 0, fixed-width center, one SUCCESS entry, optional payload,
    # two heightmap types with absent maps, and no blob hash.
    return b"\x00\x00" + bytes(12) + b"\x01\x00\x00\x00\x01\x01" + string(data) + bytes(5)


@pytest.mark.parametrize(("old", "new"), BLOCKS)
def test_existing_blocks_map_both_ways(old: int, new: int) -> None:
    assert Translator(2168, 2193).translate(21, update(old)) == update(new)
    assert Translator(2193, 2168).translate(21, update(new)) == update(old)


@pytest.mark.parametrize("corner", [705466535, 2879325999, 1289467210, 357105324])
def test_all_new_corner_shapes_return_to_the_old_stair(corner: int) -> None:
    assert Translator(2193, 2168).translate(21, update(corner)) == update(3240922889)


@pytest.mark.parametrize("version", [8, 9])
@pytest.mark.parametrize("bits", [0, 1, 2, 3, 4, 5, 6, 8, 16])
def test_chunk_palettes_preserve_indices_extra_layers_and_trailing_data(version: int, bits: int) -> None:
    old, new = BLOCKS[0]
    prefix = bytes([version, 2]) + (b"\xfc" if version == 9 else b"")
    words = 0 if bits == 0 else (4096 + 32 // bits - 1) // (32 // bits)
    # All block indices are zero. A second singleton storage represents waterlogging.
    storage = bytes([(bits << 1) | 1]) + bytes(words * 4) + (varint(1) if bits else b"")
    tail = b"\x01" + varint(2150698529) + b"biome-and-block-actor-bytes"
    before = prefix + storage + varint(old) + tail
    after = prefix + storage + varint(new) + tail
    for packet_id, wrap in ((58, level_chunk), (174, subchunk_packet)):
        assert Translator(2168, 2193).translate(packet_id, wrap(before)) == wrap(after)
        assert Translator(2193, 2168).translate(packet_id, wrap(after)) == wrap(before)


def test_inventory_items_map_and_keep_their_contents() -> None:
    def slot(runtime_id: int) -> bytes:
        item = struct.pack("<hH", 53, 32) + b"\x00\x00" + uvarint(runtime_id) + string(b"item-nbt")
        return b"\x00\x00\x00\x00" + item

    old, new = BLOCKS[0]
    assert Translator(2168, 2193).translate(50, slot(old)) == slot(new)
    assert Translator(2193, 2168).translate(50, slot(new)) == slot(old)


@pytest.mark.parametrize("direction", [(2168, 2193), (2193, 2168)])
def test_client_cache_is_disabled_when_block_palettes_differ(direction: tuple[int, int]) -> None:
    assert Translator(*direction).translate(129, b"\x01") == b"\x00"


@pytest.mark.parametrize("direction", [(2168, 2169), (2193, 2193)])
def test_matching_palettes_keep_cache_and_block_bytes(direction: tuple[int, int]) -> None:
    translator = Translator(*direction)
    assert 21 not in translator.actions
    assert 129 not in translator.actions
    assert translator.translate(129, b"\x01") == b"\x01"
    assert translator.translate(21, update(BLOCKS[0][0])) == update(BLOCKS[0][0])


@pytest.mark.parametrize("runtime_id", [0, 2150698529, 0x12345678])
def test_unchanged_and_custom_block_ids_are_preserved(runtime_id: int) -> None:
    assert Translator(2168, 2193).translate(21, update(runtime_id)) == update(runtime_id)


@pytest.mark.parametrize("data", [b"", b"\x09", b"\x09\x01\x00\x03", b"\x09\x01\x00\x0f"])
def test_truncated_and_invalid_chunk_palettes_fail_cleanly(data: bytes) -> None:
    with pytest.raises(TranslationError):
        Translator(2168, 2193).translate(58, level_chunk(data))


def test_block_updates_are_translated_even_without_a_wire_schema_change() -> None:
    translator = Translator(2168, 2193)
    for packet_id in (21, 50, 58, 110, 129, 145, 172, 174):
        assert translator.actions[packet_id] is Action.TRANSLATE


@pytest.mark.parametrize(("position", "expected"), [((1, 0, 0), 1289467210), ((-1, 0, 0), 705466535)])
def test_stair_shape_uses_perpendicular_neighbors(position: tuple[int, int, int], expected: int) -> None:
    world = {(0, 0, 0): 3240922889, position: 3179448066}
    result, _ = Translator(2168, 2193).translate_world(
        21, update(3240922889), lambda d, x, y, z: world.get((x, y, z), 0), 0
    )
    assert result == update(expected)


def test_fence_connects_and_disconnects_on_neighbor_updates() -> None:
    world = {(0, 0, 0): 1997655867, (1, 0, 0): 1997655867}
    translator = Translator(2168, 2193)

    def lookup(d: int, x: int, y: int, z: int) -> int:
        return world.get((x, y, z), 0)

    result, neighbors = translator.translate_world(21, update(1997655867), lookup, 0)
    assert result == update(3720363220)  # East connection.
    assert len(neighbors) == 1  # The other fence must also be refreshed.
    del world[(1, 0, 0)]
    result, neighbors = translator.translate_world(21, update(1997655867), lookup, 0)
    assert result == update(425458381)
    assert not neighbors


def test_uniform_stair_storage_can_expand_for_a_corner_across_the_chunk_boundary() -> None:
    old = 3240922889
    before = b"\x09\x01\x00\x01" + varint(old)

    def lookup(d: int, x: int, y: int, z: int) -> int:
        assert d == 0
        if (x, y, z) == (16, 0, 15):
            return 3179448066
        return old if 0 <= x < 16 and 0 <= y < 16 and 0 <= z < 16 else 0

    # Index order is x/z/y. Only the stair at (15, 0, 15) has an outside corner.
    words = bytearray(512)
    i = (15 << 8) | (15 << 4)
    struct.pack_into("<I", words, (i // 32) * 4, 1 << (i % 32))
    after = b"\x09\x01\x00\x03" + words + varint(2) + varint(4172483059) + varint(1289467210)
    result, neighbors = Translator(2168, 2193).translate_world(58, level_chunk(before), lookup, 0)
    assert result == level_chunk(after)
    assert not neighbors
