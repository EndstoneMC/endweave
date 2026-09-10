"""The translation engine's Python surface: version lookup, the per-ID verdicts, and carrying a payload."""

from __future__ import annotations

import pytest

from endweave._pipeline import (
    UNKNOWN,
    Action,
    Session,
    TranslationError,
    Translator,
    packet_name,
    resolve,
    supported_versions,
)

OLD = 2168
NEW = 2192
ALIAS = 2169  # 1.26.45, which puts every modelled packet on the wire exactly as OLD does
UNSUPPORTED = 975  # a registered protocol the engine has no tables for

CONTAINER_CLOSE = 47
INVENTORY_TRANSACTION = 30
SET_PLAYER_FURNACE_OPTIONS = 351  # OLD has no such packet, so NEW towards OLD drops it
LOGIN = 1  # unchanged between the two, so neither table names it

# Payloads of zeroes that both ends read as a whole packet.
EMPTY_TRANSACTION = b"\x00\x00\x00\x00"
EMPTY_CONTAINER_CLOSE = b"\x00\x00\x00"


@pytest.fixture
def session() -> Session:
    return Session()


class TestVersions:
    def test_lists_what_the_engine_carries_oldest_first(self) -> None:
        versions = supported_versions()

        assert versions == sorted(versions)
        assert {OLD, NEW} <= set(versions)

    def test_resolves_a_carried_version_to_itself(self) -> None:
        assert resolve(OLD) == OLD

    def test_resolves_a_wire_identical_version_to_the_one_it_speaks(self) -> None:
        assert resolve(ALIAS) == OLD

    def test_does_not_resolve_a_version_it_cannot_carry(self) -> None:
        assert resolve(UNSUPPORTED) == UNKNOWN


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

    def test_refuses_a_version_it_cannot_carry(self) -> None:
        with pytest.raises(ValueError):
            Translator(UNSUPPORTED, OLD)


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
    def test_carries_a_packet_the_two_versions_disagree_on(self, session: Session) -> None:
        translated = Translator(NEW, OLD).translate(session, INVENTORY_TRANSACTION, EMPTY_TRANSACTION)

        assert translated is not None
        assert translated != EMPTY_TRANSACTION

    def test_carries_a_packet_back_to_where_it_came_from(self, session: Session) -> None:
        there = Translator(NEW, OLD).translate(session, CONTAINER_CLOSE, EMPTY_CONTAINER_CLOSE)

        assert there is not None
        assert Translator(OLD, NEW).translate(session, CONTAINER_CLOSE, there) == EMPTY_CONTAINER_CLOSE

    def test_hands_back_a_packet_it_does_not_name(self, session: Session) -> None:
        assert Translator(NEW, OLD).translate(session, LOGIN, b"\x07") == b"\x07"

    def test_refuses_a_packet_the_other_side_has_no_room_for(self, session: Session) -> None:
        assert Translator(NEW, OLD).translate(session, SET_PLAYER_FURNACE_OPTIONS, b"\x00") is None

    def test_reports_a_payload_that_does_not_decode(self, session: Session) -> None:
        with pytest.raises(TranslationError) as raised:
            Translator(NEW, OLD).translate(session, INVENTORY_TRANSACTION, b"")

        assert raised.value.packet_id == INVENTORY_TRANSACTION
        assert raised.value.stage == "translate"

    def test_reports_a_payload_with_bytes_left_over(self, session: Session) -> None:
        with pytest.raises(TranslationError):
            Translator(NEW, OLD).translate(session, CONTAINER_CLOSE, EMPTY_CONTAINER_CLOSE + b"\x00")
