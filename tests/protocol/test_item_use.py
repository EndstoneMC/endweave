"""An item use carried to a version that cannot name the hand it was made with."""

from __future__ import annotations

import struct

from endweave._pipeline import Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

INVENTORY_TRANSACTION = 30
PLAYER_AUTH_INPUT = 144

MAINHAND = 0
OFFHAND = 1
PERFORM_ITEM_INTERACTION = 34
ITEM_USE = 2  # the transaction's variant index
AIR = struct.pack("<hH", 0, 0) + b"\x00\x00\x00\x00"


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


def item_use(hand: int) -> bytes:
    """Placing against the top of the block at the origin from the first hotbar slot."""
    return (
        uvarint(0)
        + varint(0)
        + b"\x01"
        + varint(0) * 3
        + b"\x01"
        + varint(0)
        + bytes([hand])
        + AIR
        + bytes(24)
        + uvarint(0)
        + b"\x00\x00"
    )


def inventory_transaction(hand: int) -> bytes:
    return varint(0) + b"\x00" + uvarint(ITEM_USE) + item_use(hand)


def auth_input(hand: int | None) -> bytes:
    """A tick of input standing still, with an item use made in `hand` or none."""
    if hand is None:
        flags, use = uvarint(0), b"\x00"
    else:
        flags, use = uvarint(1) + varint(PERFORM_ITEM_INTERACTION), b"\x01" + varint(0) + b"\x00" + item_use(hand)
    return (
        bytes(32)
        + flags
        + uvarint(1)
        + uvarint(0)
        + varint(0)
        + bytes(8)
        + uvarint(0)
        + bytes(12)
        + use
        + bytes(4)
        + bytes(28)
    )


class TestInventoryTransaction:
    def test_carries_a_main_hand_use_there_and_back(self) -> None:
        there = Translator(NEW, OLD).translate(INVENTORY_TRANSACTION, inventory_transaction(MAINHAND))

        assert there is not None
        assert Translator(OLD, NEW).translate(INVENTORY_TRANSACTION, there) == inventory_transaction(MAINHAND)

    def test_drops_an_off_hand_use_the_older_version_cannot_name(self) -> None:
        assert Translator(NEW, OLD).translate(INVENTORY_TRANSACTION, inventory_transaction(OFFHAND)) is None

    def test_drops_an_off_hand_use_across_every_hop(self) -> None:
        assert Translator(NEWEST, OLD).translate(INVENTORY_TRANSACTION, inventory_transaction(OFFHAND)) is None


class TestPlayerAuthInput:
    def test_carries_a_main_hand_use_there_and_back(self) -> None:
        there = Translator(NEW, OLD).translate(PLAYER_AUTH_INPUT, auth_input(MAINHAND))

        assert there is not None
        assert Translator(OLD, NEW).translate(PLAYER_AUTH_INPUT, there) == auth_input(MAINHAND)

    def test_keeps_the_tick_but_drops_an_off_hand_use(self) -> None:
        there = Translator(NEW, OLD).translate(PLAYER_AUTH_INPUT, auth_input(OFFHAND))

        assert there is not None
        assert Translator(OLD, NEW).translate(PLAYER_AUTH_INPUT, there) == auth_input(None)
