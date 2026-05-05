"""Central registry of known Bedrock protocol versions.

New versions are added here first. Tools and plugin both reference this
single source of truth.
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class ProtocolVersion:
    """A known Bedrock protocol version.

    Attributes:
        protocol: Numeric protocol ID (e.g. 924).
        minecraft_version: Primary Minecraft version string (e.g. "1.26.0").
        included_versions: All Minecraft version strings sharing this protocol.
    """

    protocol: int  # e.g. 924
    minecraft_version: str  # e.g. "1.26.0"
    included_versions: frozenset[str] = frozenset()  # all MC versions sharing this protocol

    @property
    def release_tag(self) -> str:
        """Derive the BedrockProtocol repo branch tag (e.g. "r26_u0").

        Convention: ``r{minor}_u{patch // 10}`` from ``1.{minor}.{patch}``.
        """
        parts = self.minecraft_version.split(".")
        minor, patch = parts[1], int(parts[2])
        return f"r{minor}_u{patch // 10}"


# Registry -- add new versions here
v1_21_120 = ProtocolVersion(859, "1.21.120", frozenset({"1.21.120"}))
v1_21_124 = ProtocolVersion(860, "1.21.124", frozenset({"1.21.124"}))
v1_21_130 = ProtocolVersion(898, "1.21.130", frozenset({"1.21.130", "1.21.131", "1.21.132"}))
v1_26_0 = ProtocolVersion(924, "1.26.0", frozenset({"1.26.0", "1.26.1", "1.26.2", "1.26.3"}))
v1_26_10 = ProtocolVersion(944, "1.26.10", frozenset({"1.26.10", "1.26.11", "1.26.12", "1.26.13"}))
v1_26_20 = ProtocolVersion(975, "1.26.20", frozenset({"1.26.20"}))

VERSIONS: dict[int, ProtocolVersion] = {
    v.protocol: v for v in [v1_21_120, v1_21_124, v1_21_130, v1_26_0, v1_26_10, v1_26_20]
}

# Reverse lookup: MC version string -> ProtocolVersion
_VERSION_BY_NAME: dict[str, ProtocolVersion] = {name: v for v in VERSIONS.values() for name in v.included_versions}


def get_version_by_name(mc_version: str) -> ProtocolVersion | None:
    """Look up a ProtocolVersion by Minecraft version string.

    Args:
        mc_version: Minecraft version string (e.g. "1.26.0").

    Returns:
        The matching ProtocolVersion, or None if not registered.
    """
    return _VERSION_BY_NAME.get(mc_version)
