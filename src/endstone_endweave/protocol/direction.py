"""Packet direction enum for serverbound/clientbound classification.

See Also:
    com.viaversion.viaversion.api.protocol.packet.Direction
"""

from enum import Enum


class Direction(Enum):
    SERVERBOUND = "serverbound"
    CLIENTBOUND = "clientbound"
