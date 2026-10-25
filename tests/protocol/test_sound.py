"""ClientboundUpdateSoundData's event, which 2193 writes once per slot and 2225 once."""

from __future__ import annotations

import struct

from endweave._pipeline import Translator

OLD = 2168
NEW = 2193
NEWEST = 2225

CLIENTBOUND_UPDATE_SOUND_DATA = 348

HANDLE = struct.pack("<Q", 42)
STOP = 0
SET_VOLUME = 1
FADE = 3
RESUME = 6


def event(case: int, *values: float) -> bytes:
    return bytes([case]) + b"".join(struct.pack("<f", value) for value in values)


def slots(*events: bytes) -> bytes:
    return HANDLE + b"".join(events)


def tagged(case: int, *values: float) -> bytes:
    return HANDLE + bytes([case]) + event(case, *values)


class TestUpdateSoundData:
    def test_gives_a_newer_client_the_event_of_every_slot(self) -> None:
        payload = slots(*[event(SET_VOLUME, 0.5)] * 7)

        assert Translator(NEW, NEWEST).translate(CLIENTBOUND_UPDATE_SOUND_DATA, payload) == tagged(SET_VOLUME, 0.5)

    def test_gives_a_newer_client_the_last_slot_as_bds_keeps_it(self) -> None:
        payload = slots(*[event(STOP)] * 6, event(RESUME))

        assert Translator(NEW, NEWEST).translate(CLIENTBOUND_UPDATE_SOUND_DATA, payload) == tagged(RESUME)

    def test_writes_an_older_client_the_event_into_every_slot(self) -> None:
        payload = tagged(FADE, 1.5, 0.25)

        assert Translator(NEWEST, NEW).translate(CLIENTBOUND_UPDATE_SOUND_DATA, payload) == slots(
            *[event(FADE, 1.5, 0.25)] * 7
        )

    def test_carries_the_event_across_every_hop(self) -> None:
        payload = slots(*[event(RESUME)] * 7)

        assert Translator(OLD, NEWEST).translate(CLIENTBOUND_UPDATE_SOUND_DATA, payload) == tagged(RESUME)
        assert Translator(NEWEST, OLD).translate(CLIENTBOUND_UPDATE_SOUND_DATA, tagged(RESUME)) == payload
