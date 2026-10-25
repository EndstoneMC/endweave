"""What a client is told of the world it joins: the block definitions StartGame carries."""

from __future__ import annotations

from endweave._pipeline import Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

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
NO_EXPERIMENTS = bytes(5)
EMPTY_COMPOUND = b"\x0a\x00\x00"

CONCRETE_STAIRS = "minecraft:white_concrete_stairs"  # NEW has it, OLD lacks it
WOOL_STAIRS = "minecraft:white_wool_stairs"  # OLD has it behind y_2026_drop_3
CUSTOM = "test:block"  # in no vanilla palette, as an add-on block is


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


def start_game(*blocks: str, version: int = NEW) -> bytes:
    # NEWEST writes its editor level migration version ahead of the day-cycle time.
    migration = b"\x00" if version == NEWEST else b""
    properties = uvarint(len(blocks)) + b"".join(string(name) + EMPTY_COMPOUND for name in blocks)
    return SETTINGS_HEAD + migration + SETTINGS_TAIL + NO_EXPERIMENTS + BEFORE_BLOCKS + properties + AFTER_BLOCKS


class TestBlockDefinitions:
    def test_leaves_out_a_block_type_the_client_lacks(self) -> None:
        translated = Translator(NEW, OLD).translate(START_GAME, start_game(CONCRETE_STAIRS, WOOL_STAIRS, CUSTOM))

        assert translated == start_game(WOOL_STAIRS, CUSTOM, version=OLD)

    def test_leaves_out_a_block_type_the_client_lacks_across_every_hop(self) -> None:
        payload = start_game(CONCRETE_STAIRS, WOOL_STAIRS, version=NEWEST)

        assert Translator(NEWEST, OLD).translate(START_GAME, payload) == start_game(WOOL_STAIRS, version=OLD)

    def test_keeps_every_definition_for_a_newer_client(self) -> None:
        payload = start_game(WOOL_STAIRS, CUSTOM, version=OLD)

        assert Translator(OLD, NEW).translate(START_GAME, payload) == start_game(WOOL_STAIRS, CUSTOM)
