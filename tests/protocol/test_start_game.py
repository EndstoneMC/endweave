"""What a client is told of the world it joins: the block definitions StartGame carries, and the
experiments StartGame and ResourcePackStack turn on."""

from __future__ import annotations

import struct

from endweave._pipeline import Action, Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

RESOURCE_PACK_STACK = 7
START_GAME = 11

# bedrock-protocol's 2168 StartGame golden, split around its experiments and its block definitions.
SETTINGS_HEAD = bytes.fromhex(
    "0202000000803f0000004000004040000080400000a040070000000000000000000000020000040080010001000000"
)
SETTINGS_TAIL = bytes.fromhex(
    "00000000000000000000000000010104040100020f646f6461796c696768746379636c65000101156d6178636f6d6d616e64"
    "636861696e6c656e6774680002feff0100"
)
BEFORE_BLOCKS = bytes.fromhex(
    "000001040000000000000000000000000006312e32312e3000000000000000000000000000000000036c766c05776f726c64"
    "00005000640000000000000000"
)
AFTER_BLOCKS = bytes.fromhex("000106312e32312e300a00000000000000000000000000000000000000000000000000000001000000000000")
EMPTY_COMPOUND = b"\x0a\x00\x00"

CONCRETE_STAIRS = "minecraft:white_concrete_stairs"  # NEW has it, OLD lacks it
WOOL_STAIRS = "minecraft:white_wool_stairs"  # OLD has it behind y_2026_drop_3
CUSTOM = "test:block"  # in no vanilla palette, as an add-on block is

DROP_3 = ("y_2026_drop_3", True)  # NEW ships it as base game, OLD gates it behind this
GAMETEST = ("gametest", True)


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


def experiments(*toggles: tuple[str, bool], ever_toggled: bool = False) -> bytes:
    entries = b"".join(string(name) + bytes([enabled]) for name, enabled in toggles)
    return struct.pack("<I", len(toggles)) + entries + bytes([ever_toggled])


NONE = experiments()
DROP_3_ON = experiments(DROP_3, ever_toggled=True)


def start_game(*blocks: str, settings: bytes = NONE, version: int = NEW) -> bytes:
    # NEWEST writes its editor level migration version ahead of the day-cycle time.
    migration = b"\x00" if version == NEWEST else b""
    properties = uvarint(len(blocks)) + b"".join(string(name) + EMPTY_COMPOUND for name in blocks)
    return SETTINGS_HEAD + migration + SETTINGS_TAIL + settings + BEFORE_BLOCKS + properties + AFTER_BLOCKS


def resource_pack_stack(settings: bytes) -> bytes:
    return b"\x01" + uvarint(0) + string("1.26.50") + settings + b"\x00"


class TestBlockDefinitions:
    def test_leaves_out_a_block_type_the_client_lacks(self) -> None:
        translated = Translator(NEW, OLD).translate(START_GAME, start_game(CONCRETE_STAIRS, WOOL_STAIRS, CUSTOM))

        assert translated == start_game(WOOL_STAIRS, CUSTOM, settings=DROP_3_ON, version=OLD)

    def test_leaves_out_a_block_type_the_client_lacks_across_every_hop(self) -> None:
        translated = Translator(NEWEST, OLD).translate(START_GAME, start_game(CONCRETE_STAIRS, version=NEWEST))

        assert translated == start_game(settings=DROP_3_ON, version=OLD)

    def test_keeps_every_definition_for_a_newer_client(self) -> None:
        payload = start_game(WOOL_STAIRS, CUSTOM, version=OLD)

        assert Translator(OLD, NEW).translate(START_GAME, payload) == start_game(WOOL_STAIRS, CUSTOM)


class TestExperiments:
    def test_turns_on_the_experiment_an_older_client_gates_the_base_game_behind(self) -> None:
        payload = start_game(settings=experiments(GAMETEST, ever_toggled=True))

        translated = Translator(NEW, OLD).translate(START_GAME, payload)

        assert translated == start_game(settings=experiments(GAMETEST, DROP_3, ever_toggled=True), version=OLD)

    def test_turns_on_an_experiment_the_server_lists_as_off_without_listing_it_twice(self) -> None:
        payload = start_game(settings=experiments(("y_2026_drop_3", False)))

        assert Translator(NEW, OLD).translate(START_GAME, payload) == start_game(settings=DROP_3_ON, version=OLD)

    def test_turns_it_on_across_every_hop(self) -> None:
        translated = Translator(NEWEST, OLD).translate(START_GAME, start_game(version=NEWEST))

        assert translated == start_game(settings=DROP_3_ON, version=OLD)

    def test_leaves_the_experiments_alone_for_a_newer_client(self) -> None:
        payload = start_game(settings=experiments(GAMETEST, ever_toggled=True), version=OLD)

        assert Translator(OLD, NEW).translate(START_GAME, payload) == payload

    def test_turns_it_on_in_the_resource_pack_stack(self) -> None:
        translated = Translator(NEW, OLD).translate(RESOURCE_PACK_STACK, resource_pack_stack(experiments(GAMETEST)))

        assert translated == resource_pack_stack(experiments(GAMETEST, DROP_3, ever_toggled=True))

    def test_turns_it_on_in_the_resource_pack_stack_across_every_hop(self) -> None:
        translated = Translator(NEWEST, OLD).translate(RESOURCE_PACK_STACK, resource_pack_stack(NONE))

        assert translated == resource_pack_stack(DROP_3_ON)

    def test_leaves_the_resource_pack_stack_alone_where_no_hop_names_an_experiment(self) -> None:
        assert Translator(NEW, OLD).actions[RESOURCE_PACK_STACK] is Action.TRANSLATE
        assert RESOURCE_PACK_STACK not in Translator(OLD, NEW).actions
        assert RESOURCE_PACK_STACK not in Translator(NEWEST, NEW).actions
