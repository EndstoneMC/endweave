"""Known Bedrock protocol versions.

A translation of ViaVersion's ``ProtocolVersion`` for Bedrock Edition. Two Java
concepts are gone: snapshot protocol ids, which Bedrock does not have, and
version types, which only exist because the Java protocol id reset several
times. Bedrock ids only ever go up, so ordering is by protocol id alone.
"""

import functools
from collections.abc import Iterator
from dataclasses import dataclass

__all__ = [
    "UNKNOWN",
    "ProtocolVersion",
    "SubVersionRange",
    "get_by_name",
    "get_protocol",
    "get_protocols",
    "is_registered",
    "register",
]


@dataclass(frozen=True)
class SubVersionRange:
    """A run of consecutive Minecraft versions sharing one protocol id.

    ``SubVersionRange("1.26", 0, 3)`` covers 1.26.0 through 1.26.3.

    Attributes:
        base_version: Version prefix shared by the whole range, e.g. "1.26".
        range_from: Lowest included patch number.
        range_to: Highest included patch number, greater than ``range_from``.
    """

    base_version: str
    range_from: int
    range_to: int

    def __post_init__(self) -> None:
        if self.range_from < 0:
            raise ValueError(f"range_from must not be negative: {self.range_from}")
        if self.range_to <= self.range_from:
            raise ValueError(f"range_to must be greater than range_from: {self.range_to} <= {self.range_from}")

    def __iter__(self) -> Iterator[str]:
        for patch in range(self.range_from, self.range_to + 1):
            yield f"{self.base_version}.{patch}"


@functools.total_ordering
class ProtocolVersion:
    """A Bedrock protocol id and the Minecraft versions that speak it."""

    __slots__ = ("included_versions", "known", "name", "version")

    def __init__(
        self,
        version: int,
        name: str,
        version_range: SubVersionRange | None = None,
        *,
        known: bool = True,
    ) -> None:
        """Create a protocol version.

        A hyphenated name such as "1.26.0-1.26.3" derives its own range, so
        ``version_range`` is only needed for names that cannot be read that way,
        wildcards among them.

        Args:
            version: Numeric protocol id, e.g. 924.
            name: Version name, e.g. "1.26.0" or "1.26.0-1.26.3".
            version_range: Minecraft versions covered, when the name does not say.
            known: False for placeholders standing in for unregistered ids.

        Raises:
            ValueError: If the name is a wildcard or an underivable range and no range is given.
        """
        if version_range is None and name.endswith(".x"):
            raise ValueError(f"wildcard name needs a version range: {name}")

        if version_range is None and "-" in name:
            first, _, last = name.partition("-")
            base_version, _, range_from = first.rpartition(".")
            last_base, _, range_to = last.rpartition(".")
            if base_version != last_base or not range_from.isdigit() or not range_to.isdigit():
                raise ValueError(f"cannot derive a version range from {name}, pass one explicitly")
            version_range = SubVersionRange(base_version, int(range_from), int(range_to))

        self.version = version
        self.name = name
        self.known = known
        self.included_versions = frozenset(version_range) if version_range is not None else frozenset({name})

    @property
    def is_range(self) -> bool:
        """Whether this covers more than one Minecraft version."""
        return len(self.included_versions) != 1

    @property
    def is_version_wildcard(self) -> bool:
        """Whether the name covers a whole minor line, e.g. "1.26.x"."""
        return self.name.endswith(".x")

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, ProtocolVersion):
            return NotImplemented
        return self.version == other.version

    def __lt__(self, other: "ProtocolVersion") -> bool:
        if not isinstance(other, ProtocolVersion):
            return NotImplemented
        return self.version < other.version

    def __hash__(self) -> int:
        return hash(self.version)

    def __str__(self) -> str:
        return f"{self.name} ({self.version})"

    def __repr__(self) -> str:
        return f"ProtocolVersion(version={self.version}, name={self.name!r})"


_VERSIONS: dict[int, ProtocolVersion] = {}
_VERSION_LIST: list[ProtocolVersion] = []


def register(version: int, name: str, version_range: SubVersionRange | None = None) -> ProtocolVersion:
    """Register a protocol version.

    Args:
        version: Numeric protocol id, e.g. 924.
        name: Version name, e.g. "1.26.0" or "1.26.0-1.26.3".
        version_range: Minecraft versions covered, when the name does not say.

    Returns:
        The registered protocol version.
    """
    protocol_version = ProtocolVersion(version, name, version_range)
    _VERSIONS[version] = protocol_version
    _VERSION_LIST.append(protocol_version)
    _VERSION_LIST.sort()
    return protocol_version


def is_registered(version: int) -> bool:
    """Whether a protocol id is registered.

    Args:
        version: Numeric protocol id.

    Returns:
        True if the id is registered.
    """
    return version in _VERSIONS


def get_protocol(version: int) -> ProtocolVersion:
    """Look up a protocol id, registered or not.

    Args:
        version: Numeric protocol id.

    Returns:
        The registered protocol version, or a placeholder whose ``known`` is False.
    """
    protocol_version = _VERSIONS.get(version)
    if protocol_version is not None:
        return protocol_version
    return ProtocolVersion(version, f"Unknown ({version})", known=False)


def get_protocols() -> list[ProtocolVersion]:
    """Return the registered protocol versions, oldest first."""
    return list(_VERSION_LIST)


def get_by_name(name: str) -> ProtocolVersion | None:
    """Look up a protocol version by Minecraft version name.

    Accepts registered names such as "1.26.0-1.26.3" as well as the individual
    versions a range covers.

    Args:
        name: Minecraft version string, e.g. "1.26.2".

    Returns:
        The matching protocol version, or None if no registered version covers it.
    """
    for protocol_version in _VERSION_LIST:
        if protocol_version.name == name or name in protocol_version.included_versions:
            return protocol_version
    return None


UNKNOWN = ProtocolVersion(-1, "UNKNOWN", known=False)

v1_21_120 = register(859, "1.21.120")
v1_21_124 = register(860, "1.21.124")
v1_21_130 = register(898, "1.21.130-1.21.132")
v1_26_0 = register(924, "1.26.0-1.26.3")
v1_26_10 = register(944, "1.26.10-1.26.13")
v1_26_20 = register(975, "1.26.20")
v1_26_30 = register(1001, "1.26.30-1.26.32")
