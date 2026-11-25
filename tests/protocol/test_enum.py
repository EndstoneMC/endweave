"""Enum values carried across: by the codec, by an EnumPolicy where the destination lacks one, and by
the actor rewriters where actor data carries them untyped."""

from __future__ import annotations

from endweave._pipeline import Action, Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

DISCONNECT = 5
SET_ACTOR_DATA = 39
CONTAINER_OPEN = 46
CONTAINER_REGISTRY_CLEANUP = 317

CHEST = 0
DATA_DRIVEN_CONTAINER = 37  # NEW and NEWEST have it, OLD lacks it
DISC_PRESS = 38  # NEWEST alone has it

UNKNOWN_REASON = 0
NEWEST_REASON = 150  # NEWEST alone has it

ANVIL_INPUT_CONTAINER = 0
DISC_PRESS_BLOCK_CONTAINER = 67  # NEWEST alone has it

HEARTBEAT_SOUND_EVENT = 126
INTERACTION_RESULT = 141  # NEWEST alone has this actor data key
DATA_ITEM_INT = 2
ICE_BALL_BREAK = 614  # NEWEST alone has it, at the number NEW gives Undefined
UNDEFINED_SOUND = {NEW: 614, NEWEST: 615}


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


def container_open(container_type: int) -> bytes:
    return b"\x01" + bytes([container_type]) + varint(0) + varint(0) + varint(0) + varint(0)


def disconnect(reason: int) -> bytes:
    """A reason with no message, which BDS sends as the second arm."""
    return varint(reason) + uvarint(1)


def cleanup(*names: int) -> bytes:
    return uvarint(len(names)) + b"".join(bytes([name]) + b"\x00" for name in names)


def int_entry(key: int, value: int) -> bytes:
    return uvarint(key) + uvarint(DATA_ITEM_INT) + bytes([DATA_ITEM_INT]) + varint(value)


def set_actor_data(*entries: bytes) -> bytes:
    return uvarint(7) + uvarint(len(entries)) + b"".join(entries) + uvarint(0) + uvarint(0) + uvarint(0)


class TestPolicy:
    def test_carries_a_value_both_versions_have_as_it_is(self) -> None:
        assert Translator(NEWEST, NEW).translate(CONTAINER_OPEN, container_open(CHEST)) == container_open(CHEST)

    def test_drops_a_container_the_older_version_has_no_screen_for(self) -> None:
        assert Translator(NEWEST, NEW).translate(CONTAINER_OPEN, container_open(DISC_PRESS)) is None
        assert Translator(NEW, OLD).translate(CONTAINER_OPEN, container_open(DATA_DRIVEN_CONTAINER)) is None

    def test_opens_a_container_the_newer_version_has_too(self) -> None:
        payload = container_open(DATA_DRIVEN_CONTAINER)

        assert Translator(NEW, NEWEST).translate(CONTAINER_OPEN, payload) == payload

    def test_lands_a_reason_the_older_version_lacks_on_unknown(self) -> None:
        translated = Translator(NEWEST, NEW).translate(DISCONNECT, disconnect(NEWEST_REASON))

        assert translated == disconnect(UNKNOWN_REASON)

    def test_translates_a_packet_whose_encoding_alone_changed(self) -> None:
        assert Translator(NEWEST, NEW).actions[CONTAINER_OPEN] is Action.TRANSLATE
        assert Translator(NEW, OLD).actions[DISCONNECT] is Action.TRANSLATE


class TestContainerRegistryCleanup:
    def test_leaves_out_a_container_the_older_version_has_no_name_for(self) -> None:
        payload = cleanup(DISC_PRESS_BLOCK_CONTAINER, ANVIL_INPUT_CONTAINER)

        translated = Translator(NEWEST, NEW).translate(CONTAINER_REGISTRY_CLEANUP, payload)

        assert translated == cleanup(ANVIL_INPUT_CONTAINER)


class TestActorData:
    def test_gives_the_heartbeat_sound_the_numbers_of_the_newer_version(self) -> None:
        payload = set_actor_data(int_entry(HEARTBEAT_SOUND_EVENT, UNDEFINED_SOUND[NEW]))

        translated = Translator(NEW, NEWEST).translate(SET_ACTOR_DATA, payload)

        assert translated == set_actor_data(int_entry(HEARTBEAT_SOUND_EVENT, UNDEFINED_SOUND[NEWEST]))

    def test_gives_the_heartbeat_sound_the_numbers_of_the_older_version(self) -> None:
        payload = set_actor_data(int_entry(HEARTBEAT_SOUND_EVENT, UNDEFINED_SOUND[NEWEST]))

        translated = Translator(NEWEST, OLD).translate(SET_ACTOR_DATA, payload)

        assert translated == set_actor_data(int_entry(HEARTBEAT_SOUND_EVENT, UNDEFINED_SOUND[NEW]))

    def test_lands_a_heartbeat_sound_the_older_version_lacks_on_undefined(self) -> None:
        payload = set_actor_data(int_entry(HEARTBEAT_SOUND_EVENT, ICE_BALL_BREAK))

        translated = Translator(NEWEST, NEW).translate(SET_ACTOR_DATA, payload)

        assert translated == set_actor_data(int_entry(HEARTBEAT_SOUND_EVENT, UNDEFINED_SOUND[NEW]))

    def test_leaves_out_a_key_the_older_version_has_no_number_for(self) -> None:
        payload = set_actor_data(int_entry(INTERACTION_RESULT, 1), int_entry(HEARTBEAT_SOUND_EVENT, ICE_BALL_BREAK))

        translated = Translator(NEWEST, NEW).translate(SET_ACTOR_DATA, payload)

        assert translated == set_actor_data(int_entry(HEARTBEAT_SOUND_EVENT, UNDEFINED_SOUND[NEW]))
