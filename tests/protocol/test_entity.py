"""Actor types a client's version lacks, shown to it as their fallbacks."""

from __future__ import annotations

import struct

from endweave._pipeline import Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

ADD_ACTOR = 13
AVAILABLE_ACTOR_IDENTIFIERS = 119
LEVEL_SOUND_EVENT = 123
SYNC_ACTOR_PROPERTY = 165

FROSTBITE = "minecraft:frostbite"  # NEWEST has it, NEW lacks it and shows it as a zombie
ZOMBIE = "minecraft:zombie"
BEE = "minecraft:bee"


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


def string(value: str) -> bytes:
    return uvarint(len(value)) + value.encode()


def add_actor(actor_type: str, properties: list[tuple[int, int]], version: int) -> bytes:
    """An actor with no attributes, data or links, and the int properties given."""
    synched = uvarint(len(properties)) + b"".join(uvarint(index) + varint(value) for index, value in properties)
    payload = (
        varint(-5)
        + uvarint(5)
        + string(actor_type)
        + struct.pack("<8f", 1, 64, 2, 0, 0, 0, 0, 0)
        + struct.pack("<2f", 0, 0)
        + uvarint(0)
        + uvarint(0)
        + synched
        + uvarint(0)
        + uvarint(0)
    )
    # NEWEST adds an optional passenger block.
    return payload + b"\x00" if version >= NEWEST else payload


def nbt_string(name: str, value: str) -> bytes:
    return b"\x08" + string(name) + string(value)


def nbt_byte(name: str, value: int) -> bytes:
    return b"\x01" + string(name) + struct.pack("<b", value)


def nbt_int(name: str, value: int) -> bytes:
    return b"\x03" + string(name) + uvarint(((value << 1) ^ (value >> 31)) & 0xFFFFFFFF)


def nbt_compounds(name: str, compounds: list[bytes]) -> bytes:
    return b"\x09" + string(name) + b"\x0a" + varint(len(compounds)) + b"".join(compounds)


def nbt_root(*entries: bytes) -> bytes:
    """A root compound; the entries go in sorted by name, as BDS writes them."""
    return b"\x0a" + string("") + b"".join(entries) + b"\x00"


def identifier(name: str, rid: int) -> bytes:
    return (
        nbt_string("bid", "")
        + nbt_byte("hasspawnegg", 1)
        + nbt_string("id", name)
        + nbt_int("rid", rid)
        + nbt_byte("summonable", 1)
        + b"\x00"
    )


def available_actor_identifiers(*identifiers: bytes) -> bytes:
    return nbt_root(nbt_compounds("idlist", list(identifiers)))


def sync_actor_property(actor_type: str) -> bytes:
    flag = nbt_string("name", "minecraft:flag") + nbt_int("type", 2) + b"\x00"
    return nbt_root(nbt_compounds("properties", [flag]), nbt_string("type", actor_type))


def level_sound_event(actor_identifier: str) -> bytes:
    return (
        string("ambient")
        + struct.pack("<3f", 1, 64, 2)
        + varint(-1)
        + string(actor_identifier)
        + b"\x00\x00"
        + struct.pack("<q", -1)
        + b"\x00"
    )


class TestAddActor:
    def test_spawns_an_actor_the_client_lacks_as_its_fallback(self) -> None:
        payload = add_actor(FROSTBITE, [(0, 1)], NEWEST)

        translated = Translator(NEWEST, NEW).translate(ADD_ACTOR, payload)

        assert translated == add_actor(ZOMBIE, [], NEW)

    def test_spawns_it_as_its_fallback_two_versions_back(self) -> None:
        translated = Translator(NEWEST, OLD).translate(ADD_ACTOR, add_actor(FROSTBITE, [], NEWEST))

        assert translated == add_actor(ZOMBIE, [], OLD)

    def test_keeps_an_actor_the_client_has(self) -> None:
        translated = Translator(NEWEST, NEW).translate(ADD_ACTOR, add_actor(BEE, [(0, 1)], NEWEST))

        assert translated == add_actor(BEE, [(0, 1)], NEW)


class TestAvailableActorIdentifiers:
    def test_leaves_out_an_actor_the_client_lacks(self) -> None:
        payload = available_actor_identifiers(identifier(FROSTBITE, 155), identifier(ZOMBIE, 32))

        translated = Translator(NEWEST, NEW).translate(AVAILABLE_ACTOR_IDENTIFIERS, payload)

        assert translated == available_actor_identifiers(identifier(ZOMBIE, 32))

    def test_is_left_alone_for_a_newer_client(self) -> None:
        assert AVAILABLE_ACTOR_IDENTIFIERS not in Translator(NEW, NEWEST).actions


class TestSyncActorProperty:
    def test_drops_the_properties_of_an_actor_the_client_lacks(self) -> None:
        payload = sync_actor_property(FROSTBITE)

        assert Translator(NEWEST, NEW).translate(SYNC_ACTOR_PROPERTY, payload) is None

    def test_keeps_the_properties_of_an_actor_the_client_has(self) -> None:
        payload = sync_actor_property(BEE)

        assert Translator(NEWEST, NEW).translate(SYNC_ACTOR_PROPERTY, payload) == payload


class TestLevelSoundEvent:
    def test_plays_the_sound_of_an_actor_the_client_lacks_as_its_fallbacks(self) -> None:
        translated = Translator(NEWEST, NEW).translate(LEVEL_SOUND_EVENT, level_sound_event(FROSTBITE))

        assert translated == level_sound_event(ZOMBIE)
