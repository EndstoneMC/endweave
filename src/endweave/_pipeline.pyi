"""Compile-time protocol translation between Bedrock versions."""



PASSTHROUGH: int = 0

TRANSLATE: int = 1

CANCEL: int = 2

ACTION_TABLE_SIZE: int = 1024

UNKNOWN: int = -1

def supported_versions() -> list[int]:
    """The protocol versions the engine translates between, oldest first."""

def resolve(protocol_version: int) -> int:
    """
    The version a protocol id is translated as, applying the wire-identical aliases, or UNKNOWN where the engine does not translate it.
    """

def packet_name(packet_id: int) -> str | None:
    """The packet's name, or None where no version names that id."""

class Session:
    """
    One connection's two directions, resolved once when the client's version is known.
    """

    def __init__(self, client_version: int, server_version: int) -> None: ...

    @property
    def client_version(self) -> int: ...

    @property
    def server_version(self) -> int: ...

    @property
    def serverbound_actions(self) -> bytes:
        """
        One byte per packet id: PASSTHROUGH, TRANSLATE or CANCEL. Index it before calling in; a passthrough packet must not be touched at all, since assigning the payload back makes the server rebuild the frame.
        """

    @property
    def clientbound_actions(self) -> bytes: ...

    def translate_serverbound(self, packet_id: int, payload: bytes) -> bytes | None:
        """
        The payload as the server should read it, or None where a transform refused it.
        """

    def translate_clientbound(self, packet_id: int, payload: bytes) -> bytes | None:
        """
        The payload as the client should read it, or None where a transform refused it.
        """
