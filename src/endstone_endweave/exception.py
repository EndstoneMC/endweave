"""Structured error context for protocol translation failures.

Stores ordered key-value context entries, formats as comma-separated pairs,
and supports source tracking.

See Also:
    com.viaversion.viaversion.exception.InformativeException
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

    @property
    def message(self) -> str:
        """Format the error with all context for logging.

        Produces comma-separated "Key: Value" pairs on a single line
        after a header message.

        Returns:
            Formatted error string with context.

        See Also:
            com.viaversion.viaversion.exception.InformativeException#getMessage
        """
        parts = []
        for key, value in self._entries:
            parts.append(f"{key}: {value}")
        context = ", ".join(parts) if parts else "(no context)"
        cause_type = type(self.__cause__).__name__
        return f"Please report this on the Endweave GitHub repository\n{context}, Cause: {cause_type}: {self.__cause__}"
