"""Central registry of known Bedrock protocol versions.

New versions are added here first. Tools and plugin both reference this
single source of truth.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class ProtocolVersion:
    """A known Bedrock protocol version."""

    protocol: int  # e.g. 924
    minecraft_version: str  # e.g. "1.26.0"
    release_tag: str  # e.g. "r26_u0" (BedrockProtocol repo branch)


# Registry — add new versions here
R26_U0 = ProtocolVersion(924, "1.26.0", "r26_u0")
R26_U1 = ProtocolVersion(944, "1.26.10", "r26_u1")

VERSIONS: dict[int, ProtocolVersion] = {v.protocol: v for v in [R26_U0, R26_U1]}


def get_version(protocol: int) -> ProtocolVersion | None:
    """Look up a ProtocolVersion by protocol number."""
    return VERSIONS.get(protocol)
