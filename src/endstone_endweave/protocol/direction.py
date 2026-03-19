"""Packet direction enum, matching ViaVersion's Direction."""

from enum import Enum


class Direction(Enum):
    SERVERBOUND = "serverbound"
    CLIENTBOUND = "clientbound"
