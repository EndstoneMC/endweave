"""Known Bedrock protocol versions.

A translation of ViaVersion's ``ProtocolVersion`` for Bedrock Edition. Two Java
concepts are gone: snapshot protocol ids, which Bedrock does not have, and
version types, which only exist because the Java protocol id reset several
times. Bedrock ids only ever go up, so ordering is by protocol id alone.
"""

import functools
import re
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

    ``SubVersionRange("26", 0, 3)`` covers 26.0 through 26.3.

    Attributes:
        base_version: Version prefix shared by the whole range, e.g. "26".
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

        Names derive their own range, so ``version_range`` is only needed for a
        name none of these three shapes fits:

        * "26.20", one Minecraft version;
        * "26.0-26.3", every version between the two, inclusive;
        * "26.2x", the whole hotfix line 26.20 to 26.29.

        The wildcard is ViaVersion's, moved one digit along. Bedrock spends the
        last digit of the patch on hotfixes where Java spends a component of its
        own, so Java's "1.8.x" is Bedrock's "26.2x". Both are read the same
        way: whatever stands before the "x" is fixed, and the digit it replaces
        runs 0 through 9.

        Args:
            version: Numeric protocol id, e.g. 924.
            name: Version name, e.g. "26.0", "26.0-26.3" or "26.2x".
            version_range: Minecraft versions covered, when the name does not say.
            known: False for placeholders standing in for unregistered ids,
                whose names are never read as a range.

        Raises:
            ValueError: If the name looks like a range or a wildcard but cannot be read as one.
        """
        if known and version_range is None and "-" in name:
            first, _, last = name.partition("-")
            base_version, _, range_from_text = first.rpartition(".")
            last_base, _, range_to_text = last.rpartition(".")
            if base_version != last_base or not range_from_text.isdigit() or not range_to_text.isdigit():
                raise ValueError(f"cannot derive a version range from {name}, pass one explicitly")
            version_range = SubVersionRange(base_version, int(range_from_text), int(range_to_text))

        if known and version_range is None and name.endswith("x"):
            base_version, _, line = name[:-1].rpartition(".")
            if not base_version or (line and not line.isdigit()):
                raise ValueError(f"cannot derive a version range from {name}, pass one explicitly")
            range_from = int(line + "0") if line else 0
            version_range = SubVersionRange(base_version, range_from, range_from + 9)

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
        """Whether the name covers a whole hotfix line, e.g. "26.2x"."""
        return self.name.endswith("x")

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


_LEGACY_VERSION = re.compile(r"\b1\.(?=(?:2[6-9]|[3-9]\d|\d{3,})\.)")

_VERSIONS: dict[int, ProtocolVersion] = {}
_VERSION_LIST: list[ProtocolVersion] = []


def register(version: int, name: str, version_range: SubVersionRange | None = None) -> ProtocolVersion:
    """Register a protocol version.

    Args:
        version: Numeric protocol id, e.g. 924.
        name: Version name, e.g. "26.0", "26.0-26.3" or "26.2x".
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

    Accepts registered names such as "26.0-26.3" as well as the individual
    versions a range covers. From 26 on, the legacy "1.26.2" form reads as "26.2".

    Args:
        name: Minecraft version string, e.g. "26.2" or "1.26.2".

    Returns:
        The matching protocol version, or None if no registered version covers it.
    """
    name = _LEGACY_VERSION.sub("", name)
    for protocol_version in _VERSION_LIST:
        if protocol_version.name == name or name in protocol_version.included_versions:
            return protocol_version
    return None


UNKNOWN = ProtocolVersion(-1, "UNKNOWN", known=False)

v1_21_120 = register(859, "1.21.120-1.21.123")
v1_21_124 = register(860, "1.21.124")
v1_21_130 = register(898, "1.21.130-1.21.132")
v26_0 = register(924, "26.0-26.3")
v26_10 = register(944, "26.10-26.13")
v26_20 = register(975, "26.20")
v26_30 = register(1001, "26.30-26.32")
v26_40 = register(2168, "26.40-26.44")
v26_45 = register(2169, "26.45")
v26_50 = register(2192, "26.5x")
