"""Items a client's version lacks, kept out of its creative inventory and its recipes."""

from __future__ import annotations

import struct

from endweave._pipeline import Translator

OLD = 2168
NEW = 2193

CRAFTING_DATA = 52
CREATIVE_CONTENT = 145
ITEM_REGISTRY = 162

STICK = ("minecraft:stick", 7)
WOOL_STAIRS = ("minecraft:white_wool_stairs", 300)  # NEW has it, OLD lacks it


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


def item_registry(*items: tuple[str, int]) -> bytes:
    """Legacy items with no components."""
    return uvarint(len(items)) + b"".join(
        string(name) + struct.pack("<h", item_id) + b"\x00\x00" + b"\x0a\x00\x00" for name, item_id in items
    )


def descriptor(item: tuple[str, int]) -> bytes:
    return varint(item[1]) + struct.pack("<H", 1) + uvarint(0) + varint(0) + string("")


def creative_group(name: str, icon: tuple[str, int]) -> bytes:
    return b"\x01" + string(name) + descriptor(icon)


def creative_entry(net_id: int, item: tuple[str, int], group: int) -> bytes:
    return uvarint(net_id) + descriptor(item) + uvarint(group)


def creative_content(groups: list[bytes], entries: list[bytes]) -> bytes:
    return uvarint(len(groups)) + b"".join(groups) + uvarint(len(entries)) + b"".join(entries)


def shapeless_recipe(ingredient: str, result: tuple[str, int], net_id: int) -> bytes:
    ingredients = uvarint(1) + uvarint(1) + string("name") + string(ingredient) + varint(0) + varint(1)
    results = uvarint(1) + descriptor(result)
    return (
        string(f"recipe_{net_id}")
        + ingredients
        + results
        + bytes(16)
        + string("crafting_table")
        + varint(0)
        + b"\x00"
        + uvarint(net_id)
    )


def potion_mix(source: int, reagent: int, result: int) -> bytes:
    return varint(source) + varint(0) + varint(reagent) + varint(0) + varint(result) + varint(0)


def crafting_data(shapeless: list[bytes] = (), potions: list[bytes] = ()) -> bytes:
    return (
        b"\x00"
        + uvarint(len(shapeless))
        + b"".join(shapeless)
        + bytes(6)
        + uvarint(len(potions))
        + b"".join(potions)
        + b"\x00\x00\x01"
    )


def registered(from_version: int, to_version: int) -> Translator:
    translator = Translator(from_version, to_version)
    translator.translate(ITEM_REGISTRY, item_registry(STICK, WOOL_STAIRS))
    return translator


class TestItemRegistry:
    def test_keeps_every_item_for_an_older_client(self) -> None:
        payload = item_registry(STICK, WOOL_STAIRS)

        assert Translator(NEW, OLD).translate(ITEM_REGISTRY, payload) == payload


class TestCreativeContent:
    def test_leaves_out_a_creative_item_the_client_lacks(self) -> None:
        payload = creative_content(
            [creative_group("stairs", STICK)], [creative_entry(1, STICK, 0), creative_entry(2, WOOL_STAIRS, 0)]
        )

        translated = registered(NEW, OLD).translate(CREATIVE_CONTENT, payload)

        assert translated == creative_content([creative_group("stairs", STICK)], [creative_entry(1, STICK, 0)])

    def test_leaves_out_a_group_its_missing_items_emptied(self) -> None:
        payload = creative_content(
            [creative_group("wool", WOOL_STAIRS), creative_group("sticks", STICK)],
            [creative_entry(1, WOOL_STAIRS, 0), creative_entry(2, STICK, 1)],
        )

        translated = registered(NEW, OLD).translate(CREATIVE_CONTENT, payload)

        assert translated == creative_content([creative_group("sticks", STICK)], [creative_entry(2, STICK, 0)])

    def test_gives_a_group_with_a_missing_icon_its_first_item_left(self) -> None:
        payload = creative_content(
            [creative_group("mixed", WOOL_STAIRS)], [creative_entry(1, WOOL_STAIRS, 0), creative_entry(2, STICK, 0)]
        )

        translated = registered(NEW, OLD).translate(CREATIVE_CONTENT, payload)

        assert translated == creative_content([creative_group("mixed", STICK)], [creative_entry(2, STICK, 0)])

    def test_keeps_every_creative_item_for_a_newer_client(self) -> None:
        payload = creative_content([creative_group("wool", WOOL_STAIRS)], [creative_entry(1, WOOL_STAIRS, 0)])

        assert registered(OLD, NEW).translate(CREATIVE_CONTENT, payload) == payload


class TestCraftingData:
    def test_leaves_out_a_recipe_that_makes_an_item_the_client_lacks(self) -> None:
        payload = crafting_data([shapeless_recipe(STICK[0], WOOL_STAIRS, 1), shapeless_recipe(STICK[0], STICK, 2)])

        translated = registered(NEW, OLD).translate(CRAFTING_DATA, payload)

        assert translated == crafting_data([shapeless_recipe(STICK[0], STICK, 2)])

    def test_leaves_out_a_recipe_that_takes_an_item_the_client_lacks(self) -> None:
        payload = crafting_data([shapeless_recipe(WOOL_STAIRS[0], STICK, 1)])

        assert registered(NEW, OLD).translate(CRAFTING_DATA, payload) == crafting_data([])

    def test_leaves_out_a_potion_mix_with_an_item_the_client_lacks(self) -> None:
        payload = crafting_data(potions=[potion_mix(STICK[1], WOOL_STAIRS[1], STICK[1]), potion_mix(7, 7, 7)])

        translated = registered(NEW, OLD).translate(CRAFTING_DATA, payload)

        assert translated == crafting_data(potions=[potion_mix(7, 7, 7)])

    def test_keeps_every_recipe_for_a_newer_client(self) -> None:
        payload = crafting_data([shapeless_recipe(WOOL_STAIRS[0], WOOL_STAIRS, 1)])

        assert registered(OLD, NEW).translate(CRAFTING_DATA, payload) == payload
