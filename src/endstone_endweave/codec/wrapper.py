"""PacketWrapper -- ViaVersion-style read/transform/write packet manipulation.

Holds separate read (input) and write (output) buffers. Handlers use three
operations to transform packets field-by-field:

    passthrough(type) -- read from input, copy to output, return value
    read(type)        -- read from input, do NOT write (removes a field)
    write(type, val)  -- write to output without reading (inserts a field)

After all handlers run, the wrapper produces the final payload from its
write buffer plus any unread trailing bytes.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.codec.types import Type

if TYPE_CHECKING:
    from endstone_endweave.connection import UserConnection


class PacketWrapper:
    """Wraps a packet payload for field-level read/transform/write."""

    __slots__ = ("_reader", "_writer", "_cancelled", "_user")

    def __init__(self, payload: bytes, user: UserConnection | None = None) -> None:
        self._reader = PacketReader(payload)
        self._writer = PacketWriter()
        self._cancelled = False
        self._user = user

    def user(self) -> UserConnection:
        """Return the UserConnection associated with this packet."""
        assert self._user is not None, "No UserConnection set on this PacketWrapper"
        return self._user

    @property
    def reader(self) -> PacketReader:
        """Direct access to the underlying reader for advanced use."""
        return self._reader

    @property
    def writer(self) -> PacketWriter:
        """Direct access to the underlying writer for advanced use."""
        return self._writer

    @property
    def cancelled(self) -> bool:
        return self._cancelled

    def cancel(self) -> None:
        """Mark this packet for cancellation (will be dropped)."""
        self._cancelled = True

    def passthrough(self, field_type: Type) -> object:
        """Read a field from input and write it to output. Returns the value."""
        value = field_type.read(self._reader)
        field_type.write(self._writer, value)
        return value

    def read(self, field_type: Type) -> object:
        """Read a field from input without writing (removes field from output)."""
        return field_type.read(self._reader)

    def write(self, field_type: Type, value: object) -> None:
        """Write a field to output without reading (inserts a new field)."""
        field_type.write(self._writer, value)

    def passthrough_all(self) -> bytes:
        """Copy all remaining input bytes to output."""
        remaining = self._reader.read_remaining()
        self._writer.write_bytes(remaining)
        return remaining

    def has_remaining(self) -> bool:
        """Check if there are unread bytes in the input."""
        return self._reader.has_remaining()

    def to_bytes(self) -> bytes:
        """Produce final payload: written output + any unread input bytes."""
        if self._reader.has_remaining():
            self._writer.write_bytes(self._reader.read_remaining())
        return self._writer.to_bytes()
