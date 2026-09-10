"""Compile-time protocol translation between Bedrock versions."""

from collections.abc import Mapping
import enum


class Action(enum.Enum):
    """
    How a translator handles a packet id. Leave ids not in `actions` alone: assigning the payload back makes the server rebuild the frame.
    """

    TRANSLATE = 1

    CANCEL = 2

def supported_versions() -> list[int]:
    """The protocol versions the engine translates between, oldest first."""

def packet_name(packet_id: int) -> str | None:
    """The packet's name, or None if no version defines that id."""

class Session:
    """
    Per-connection state. Pass the same session to both of a connection's translators.
    """

    def __init__(self) -> None: ...

class Translator:
    """The translation from one protocol version to another."""

    def __init__(self, from_version: int, to_version: int) -> None: ...

    @property
    def from_version(self) -> int: ...

    @property
    def to_version(self) -> int: ...

    @property
    def actions(self) -> Mapping[int, Action]:
        """
        The action for each packet id that needs one. Other ids pass through untouched.
        """

    def translate(self, session: Session, packet_id: int, payload: bytes) -> bytes | None:
        """The translated payload, or None if the packet was cancelled."""

class TranslationError(RuntimeError):
    packet_id: int
    stage: str
