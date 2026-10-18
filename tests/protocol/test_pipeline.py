"""The translation engine's Python surface: version lookup, the per-ID verdicts, and carrying a payload."""

from __future__ import annotations

import pytest

from endweave._pipeline import (
    Action,
    TranslationError,
    Translator,
    packet_name,
    supported_versions,
)

OLD = 2168
NEW = 2193
NEWEST = 2225
ALIAS = 2169  # 26.45, which puts every modelled packet on the wire exactly as OLD does
UNSUPPORTED = 975  # a registered protocol the engine has no tables for

CONTAINER_CLOSE = 47
INVENTORY_TRANSACTION = 30
ANIMATE = 44
COMMAND_REQUEST = 77
SET_PLAYER_FURNACE_OPTIONS = 351  # OLD has no such packet, so NEW towards OLD drops it
SET_PASSENGER_OF_BLOCK = 357  # NEWEST alone has it
LOGIN = 1  # unchanged between the two, so neither table names it

# Payloads of zeroes that both ends read as a whole packet.
EMPTY_TRANSACTION = b"\x00\x00\x00\x00"
EMPTY_CONTAINER_CLOSE = b"\x00\x00\x00"

# A critical hit on actor 7 with no swing source; NEWEST adds the hand that swung.
CRITICAL_HIT = b"\x04\x07\x00\x00\x00\x00\x00"
MAINHAND = b"\x00"
OFFHAND = b"\x01"

# An empty command from a player, on the version a client names for its newest.
LATEST_COMMAND = b"\x00\x06player" + bytes(16) + b"\x00" + bytes(8) + b"\x00\x06latest"


class TestVersions:
    def test_lists_what_the_engine_carries_oldest_first(self) -> None:
        versions = supported_versions()

        assert versions == sorted(versions)
        assert {OLD, NEW} <= set(versions)


class TestPacketNames:
    def test_names_a_packet_the_wire_carries(self) -> None:
        assert packet_name(INVENTORY_TRANSACTION) == "inventorytransaction"

    def test_has_no_name_for_an_id_no_version_uses(self) -> None:
        assert packet_name(1023) is None


class TestTranslatorLookup:
    def test_carries_a_pair_the_engine_knows(self) -> None:
        translator = Translator(NEW, OLD)

        assert (translator.from_version, translator.to_version) == (NEW, OLD)

    def test_reads_an_alias_as_the_version_it_speaks(self) -> None:
        assert Translator(ALIAS, NEW).from_version == OLD
        assert Translator(NEW, ALIAS).to_version == OLD

    def test_refuses_a_version_it_cannot_carry(self) -> None:
        with pytest.raises(ValueError):
            Translator(UNSUPPORTED, OLD)
        with pytest.raises(ValueError):
            Translator(OLD, UNSUPPORTED)


class TestActions:
    def test_names_only_the_packets_that_need_work(self) -> None:
        actions = Translator(NEW, OLD).actions

        assert actions[INVENTORY_TRANSACTION] is Action.TRANSLATE
        assert actions[SET_PLAYER_FURNACE_OPTIONS] is Action.CANCEL
        assert LOGIN not in actions

    def test_names_nothing_when_both_ends_agree(self) -> None:
        assert Translator(OLD, OLD).actions == {}

    def test_cannot_be_written_through(self) -> None:
        with pytest.raises(TypeError):
            Translator(NEW, OLD).actions[LOGIN] = Action.CANCEL  # type: ignore[index]


class TestTranslating:
    def test_carries_a_packet_the_two_versions_disagree_on(self) -> None:
        translated = Translator(NEW, OLD).translate(INVENTORY_TRANSACTION, EMPTY_TRANSACTION)

        assert translated is not None
        assert translated != EMPTY_TRANSACTION

    def test_carries_a_packet_back_to_where_it_came_from(self) -> None:
        there = Translator(NEW, OLD).translate(CONTAINER_CLOSE, EMPTY_CONTAINER_CLOSE)

        assert there is not None
        assert Translator(OLD, NEW).translate(CONTAINER_CLOSE, there) == EMPTY_CONTAINER_CLOSE

    def test_hands_back_a_packet_it_does_not_name(self) -> None:
        assert Translator(NEW, OLD).translate(LOGIN, b"\x07") == b"\x07"

    def test_refuses_a_packet_the_other_side_has_no_room_for(self) -> None:
        assert Translator(NEW, OLD).translate(SET_PLAYER_FURNACE_OPTIONS, b"\x00") is None

    def test_reports_a_payload_that_does_not_decode(self) -> None:
        with pytest.raises(TranslationError) as raised:
            Translator(NEW, OLD).translate(INVENTORY_TRANSACTION, b"")

        assert raised.value.packet_id == INVENTORY_TRANSACTION
        assert raised.value.stage == "translate"

    def test_reports_a_payload_with_bytes_left_over(self) -> None:
        with pytest.raises(TranslationError):
            Translator(NEW, OLD).translate(CONTAINER_CLOSE, EMPTY_CONTAINER_CLOSE + b"\x00")


class TestNewestHop:
    def test_carries_the_oldest_version_across_both_hops(self) -> None:
        assert (Translator(OLD, NEWEST).from_version, Translator(OLD, NEWEST).to_version) == (OLD, NEWEST)
        assert Translator(ALIAS, NEWEST).from_version == OLD

    def test_drops_a_packet_only_the_newest_version_has(self) -> None:
        assert Translator(NEWEST, NEW).actions[SET_PASSENGER_OF_BLOCK] is Action.CANCEL
        assert Translator(NEWEST, OLD).actions[SET_PASSENGER_OF_BLOCK] is Action.CANCEL

    def test_swings_the_main_hand_for_a_version_that_names_none(self) -> None:
        assert Translator(NEW, NEWEST).translate(ANIMATE, CRITICAL_HIT) == CRITICAL_HIT + MAINHAND

    def test_carries_a_main_hand_swing_back(self) -> None:
        assert Translator(NEWEST, NEW).translate(ANIMATE, CRITICAL_HIT + MAINHAND) == CRITICAL_HIT

    def test_drops_an_off_hand_swing_the_older_version_cannot_name(self) -> None:
        assert Translator(NEWEST, NEW).translate(ANIMATE, CRITICAL_HIT + OFFHAND) is None

    def test_carries_a_command_on_the_latest_version_down(self) -> None:
        assert Translator(NEWEST, NEW).translate(COMMAND_REQUEST, LATEST_COMMAND) is not None
        assert Translator(NEWEST, OLD).translate(COMMAND_REQUEST, LATEST_COMMAND) is not None
