"""Structured error context for protocol translation failures.

Aligned with ViaVersion's InformativeException: stores ordered key-value
context entries, formats as comma-separated pairs, supports source tracking.
"""

_MAX_VALUE_LENGTH = 256


class InformativeException(Exception):
    """Exception with structured key-value context for debugging translation failures.

    Stores entries as an ordered list (not a dict) to preserve insertion order
    and allow duplicate keys like "Source 0", "Source 1".

    Example::

        raise InformativeException(original).set("Direction", "SERVERBOUND").set("Packet", "START_GAME(11)")
    """

    def __init__(self, cause: Exception) -> None:
        super().__init__(str(cause))
        self.__cause__ = cause
        self._entries: list[tuple[str, str]] = []
        self._sources: int = 0
        self.should_be_printed: bool = True

    def set(self, key: str, value: object) -> "InformativeException":
        """Attach a context key-value pair. Returns self for chaining.

        Args:
            key: Context label (e.g. "Direction", "Packet ID", "Protocol").
            value: Context value (converted to str, truncated if too long).
        """
        s = str(value)
        if len(s) > _MAX_VALUE_LENGTH:
            s = s[:_MAX_VALUE_LENGTH] + "..."
        self._entries.append((key, s))
        return self

    def add_source(self, source: type) -> "InformativeException":
        """Add a source class to the context, auto-numbering.

        Args:
            source: The class where the error originated.
        """
        name = source.__qualname__
        self.set(f"Source {self._sources}", name)
        self._sources += 1
        return self

    def get_message(self) -> str:
        """Format the error with all context for logging.

        Matches ViaVersion's InformativeException.getMessage() format:
        comma-separated "Key: Value" pairs on a single line after a
        header message.

        Returns:
            Formatted error string with context.
        """
        parts = []
        for key, value in self._entries:
            parts.append(f"{key}: {value}")
        context = ", ".join(parts) if parts else "(no context)"
        cause_type = type(self.__cause__).__name__
        return (
            f"Please report this on the Endweave GitHub repository\n"
            f"{context}, Cause: {cause_type}: {self.__cause__}"
        )
