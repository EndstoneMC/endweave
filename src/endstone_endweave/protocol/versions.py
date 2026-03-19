"""Central registry of known Bedrock protocol versions.

New versions are added here first. Tools and plugin both reference this
single source of truth.
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class ProtocolVersion:
    """A known Bedrock protocol version."""

    protocol: int  # e.g. 924
    minecraft_version: str  # e.g. "1.26.0"
    release_tag: str  # e.g. "r26_u0" (BedrockProtocol repo branch)
    included_versions: frozenset[str] = (
        frozenset()
    )  # all MC versions sharing this protocol


# Registry — add new versions here
R26_U0 = ProtocolVersion(
    924, "1.26.0", "r26_u0", frozenset({"1.26.0", "1.26.1", "1.26.2", "1.26.3"})
)
R26_U1 = ProtocolVersion(944, "1.26.10", "r26_u1", frozenset({"1.26.10"}))

VERSIONS: dict[int, ProtocolVersion] = {v.protocol: v for v in [R26_U0, R26_U1]}

# Reverse lookup: MC version string -> ProtocolVersion
_VERSION_BY_NAME: dict[str, ProtocolVersion] = {
    name: v for v in VERSIONS.values() for name in v.included_versions
}


def get_version(protocol: int) -> ProtocolVersion | None:
    """Look up a ProtocolVersion by protocol number."""
    return VERSIONS.get(protocol)


def get_version_by_name(mc_version: str) -> ProtocolVersion | None:
    """Look up a ProtocolVersion by Minecraft version string."""
    return _VERSION_BY_NAME.get(mc_version)
