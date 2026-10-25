"""Mob effects a client's version lacks, kept off it."""

from __future__ import annotations

from endweave._pipeline import Action, Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

MOB_EFFECT = 28
SET_ACTOR_DATA = 39

VISIBLE_MOB_EFFECTS = 131

SPEED = 1
POISON = 19
FREEZING = 38  # NEWEST has it, NEW lacks it


def uvarint(value: int) -> bytes:
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        out.append(byte | (0x80 if value else 0))
        if not value:
            return bytes(out)


def varint(value: int) -> bytes:
    return uvarint(((value << 1) ^ (value >> 63)) & 0xFFFFFFFFFFFFFFFF)


def mob_effect(effect: int) -> bytes:
    return uvarint(7) + b"\x01" + varint(effect) + varint(0) + b"\x01" + varint(200) + uvarint(0) + b"\x00"


def visible_effects(*effects: tuple[int, bool]) -> int:
    """Packed as BDS packs them: seven bits each, the id above the ambient bit, the first highest."""
    packed = 0
    for effect, ambient in effects:
        packed = (packed << 7) | (effect << 1) | ambient
    return packed


def set_actor_data(effects: int) -> bytes:
    entry = uvarint(VISIBLE_MOB_EFFECTS) + uvarint(7) + b"\x07" + varint(effects)
    return uvarint(7) + uvarint(1) + entry + uvarint(0) + uvarint(0) + uvarint(0)


class TestMobEffect:
    def test_drops_an_effect_the_client_lacks(self) -> None:
        assert Translator(NEWEST, NEW).translate(MOB_EFFECT, mob_effect(FREEZING)) is None

    def test_drops_it_for_a_client_two_versions_back(self) -> None:
        assert Translator(NEWEST, OLD).translate(MOB_EFFECT, mob_effect(FREEZING)) is None

    def test_keeps_an_effect_the_client_has(self) -> None:
        assert Translator(NEWEST, NEW).translate(MOB_EFFECT, mob_effect(SPEED)) == mob_effect(SPEED)

    def test_is_translated_only_where_the_client_lacks_effects(self) -> None:
        assert Translator(NEWEST, NEW).actions.get(MOB_EFFECT) is Action.TRANSLATE
        assert MOB_EFFECT not in Translator(NEW, NEWEST).actions
        assert MOB_EFFECT not in Translator(NEW, OLD).actions


class TestVisibleMobEffects:
    def test_leaves_out_a_visible_effect_the_client_lacks(self) -> None:
        payload = set_actor_data(visible_effects((SPEED, False), (FREEZING, True), (POISON, True)))

        translated = Translator(NEWEST, NEW).translate(SET_ACTOR_DATA, payload)

        assert translated == set_actor_data(visible_effects((SPEED, False), (POISON, True)))

    def test_keeps_the_visible_effects_the_client_has(self) -> None:
        payload = set_actor_data(visible_effects((SPEED, True), (POISON, False)))

        assert Translator(NEWEST, NEW).translate(SET_ACTOR_DATA, payload) == payload
