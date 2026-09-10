"""Compile-time protocol translation between Bedrock versions."""

from collections.abc import Mapping
import enum


class Action(enum.Enum):
    """
    What a packet costs on a translator. An id the translator does not name costs nothing and must not be touched at all, since assigning the payload back makes the server rebuild the frame.
    """

    TRANSLATE = 1

    CANCEL = 2

def supported_versions() -> list[int]:
    """The protocol versions the engine translates between, oldest first."""

def packet_name(packet_id: int) -> str | None:
    """The packet's name, or None where no version names that id."""

class Session:
    """
    What one connection carries across its packets. Both of a connection's translators are handed the same session.
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
        The verdict for every packet id that needs one. Read it before calling in; an id it does not name is carried untouched.
        """

    def translate(self, session: Session, packet_id: int, payload: bytes) -> bytes | None:
        """
        The payload as the other side should read it, or None where the packet was refused.
        """

class TranslationError(RuntimeError):
    packet_id: int
    stage: str
