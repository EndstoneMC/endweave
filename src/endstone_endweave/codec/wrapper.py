"""PacketWrapper -- read/transform/write packet manipulation.

Holds separate read (input) and write (output) buffers. Handlers use three
operations to transform packets field-by-field:

    passthrough(type)      -- read from input, copy to output, return value
    read(type)             -- read from input, do NOT write (removes a field)
    write(type, val)       -- write to output without reading (inserts a field)
    map(old_type, new_type) -- read with old type, write with new type (converts encoding)

After all handlers run, the wrapper produces the final payload from its
write buffer plus any unread trailing bytes.

See Also:
    com.viaversion.viaversion.api.type.Types
"""

from typing import TYPE_CHECKING, TypeVar

from .reader import PacketReader
from .types import Type
from .writer import PacketWriter

if TYPE_CHECKING:
    from endstone_endweave.connection import UserConnection

_T = TypeVar("_T")


class PacketWrapper:
    """Wraps a packet payload for field-level read/transform/write.

    Attributes:
        _reader: Input buffer for reading the original packet fields.
        _writer: Output buffer for building the transformed packet.
        _cancelled: Whether this packet has been marked for cancellation.
        _user: The player connection associated with this packet, if any.
    """

    def __init__(self, payload: bytes, user: "UserConnection | None" = None) -> None:
        self._reader = PacketReader(payload)
        self._writer = PacketWriter()
        self._cancelled = False
        self._user = user

    @property
    def user(self) -> "UserConnection":
        """Return the UserConnection associated with this packet.

        Returns:
            The UserConnection for the player who sent or will receive
            this packet.

        Raises:
            RuntimeError: If no UserConnection was set on this wrapper.
        """
        if self._user is None:
            raise RuntimeError("No UserConnection set on this PacketWrapper")
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

    def passthrough(self, field_type: Type[_T]) -> _T:
        """Read a field from input and write it to output.

        Args:
            field_type: The Type descriptor for the field to copy.

        Returns:
            The deserialized value that was copied through.
        """
        value = field_type.read(self._reader)
        field_type.write(self._writer, value)
        return value

    def read(self, field_type: Type[_T]) -> _T:
        """Read a field from input without writing (removes field from output).

        Args:
            field_type: The Type descriptor for the field to consume.

        Returns:
            The deserialized value that was removed from the output.
        """
        return field_type.read(self._reader)

    def write(self, field_type: Type[_T], value: _T) -> None:
        """Write a field to output without reading (inserts a new field).

        Args:
            field_type: The Type descriptor for the field to write.
            value: The value to serialize into the output.
        """
        field_type.write(self._writer, value)

    def map(self, old_type: Type[_T], new_type: Type[_T]) -> _T:
        """Read a field with one type and write it with another.

        Args:
            old_type: The Type descriptor to read the input field.
            new_type: The Type descriptor to write the output field.

        Returns:
            The deserialized value.

        See Also:
            com.viaversion.viaversion.api.protocol.remapper.PacketHandlers#map
        """
        value = old_type.read(self._reader)
        new_type.write(self._writer, value)
        return value

    def passthrough_all(self) -> bytes:
        """Copy all remaining input bytes to output."""
        remaining = self._reader.read_remaining()
        self._writer.write_bytes(remaining)
        return remaining

    @property
    def has_remaining(self) -> bool:
        """Check if there are unread bytes in the input."""
        return self._reader.has_remaining

    def to_bytes(self) -> bytes:
        """Produce final payload: written output + any unread input bytes.

        Returns:
            The complete transformed packet payload as bytes.
        """
        if self._reader.has_remaining:
            self._writer.write_bytes(self._reader.read_remaining())
        return self._writer.to_bytes()
