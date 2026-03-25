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
        release_tag: BedrockProtocol repo branch tag (e.g. "r26_u0").
        included_versions: All Minecraft version strings sharing this protocol.
    """

    protocol: int  # e.g. 924
    minecraft_version: str  # e.g. "1.26.0"
    release_tag: str  # e.g. "r26_u0" (BedrockProtocol repo branch)
    included_versions: frozenset[str] = frozenset()  # all MC versions sharing this protocol


# Registry — add new versions here
R21_U12_120 = ProtocolVersion(859, "1.21.120", "r21_u12", frozenset({"1.21.120"}))
R21_U12_124 = ProtocolVersion(860, "1.21.124", "r21_u12", frozenset({"1.21.124"}))
R21_U13 = ProtocolVersion(898, "1.21.130", "r21_u13", frozenset({"1.21.130", "1.21.131", "1.21.132"}))
R26_U0 = ProtocolVersion(924, "1.26.0", "r26_u0", frozenset({"1.26.0", "1.26.1", "1.26.2", "1.26.3"}))
R26_U1 = ProtocolVersion(944, "1.26.10", "r26_u1", frozenset({"1.26.10"}))

VERSIONS: dict[int, ProtocolVersion] = {v.protocol: v for v in [R21_U12_120, R21_U12_124, R21_U13, R26_U0, R26_U1]}

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
