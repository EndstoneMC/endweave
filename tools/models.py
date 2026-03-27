"""Pydantic models for the protocol docs tooling pipeline.

Shared data structures used by parse_protocol_docs.py and generate_diff.py.
"""

import re

from pydantic import BaseModel


class Field(BaseModel):
    """A single field in a packet or type definition.

    Attributes:
        name: Field name (e.g. "Block Position", "X").
        type: Type name (e.g. "NetworkBlockPosition", "varint32").
        attributes: Bitmask from DOT format (512=leaf, 256=reference,
            8=list, 16=element, 2=dependency, 4=conditional).
        children: Nested child fields.
    """

    name: str
    type: str = ""
    attributes: int = 0
    children: list["Field"] = []


class PacketDefinition(BaseModel):
    """A parsed packet or type definition from protocol docs.

    Attributes:
        name: Packet/type name (e.g. "UpdateBlockPacket", "LevelSettings").
        packet_id: Numeric packet ID if this is a packet, None for sub-types.
        direction: "clientbound", "serverbound", or None if unknown.
        description: Human-readable description from protocol docs.
        fields: Top-level fields in serialization order.
    """

    name: str
    packet_id: int | None = None
    direction: str | None = None
    description: str = ""
    fields: list[Field] = []


class TypeChange(BaseModel):
    """A field type that changed between protocol versions."""

    old: str
    new: str


class PacketChanges(BaseModel):
    """Per-packet diff between two protocol versions.

    Attributes:
        packet_id: Numeric packet ID (from old version), None for sub-types.
        direction: Packet direction, None if unknown.
        added_fields: Dotted field paths only in the new version.
        removed_fields: Dotted field paths only in the old version.
        type_changes: Fields whose type changed, keyed by dotted path.
    """

    packet_id: int | None = None
    direction: str | None = None
    added_fields: list[str] = []
    removed_fields: list[str] = []
    type_changes: dict[str, TypeChange] = {}


_NODE_ID_RE = re.compile(r"^node_\d+$")


class ProtocolDiff(BaseModel):
    """Complete diff between two protocol versions.

    Attributes:
        old_protocol: Protocol number of the older version.
        new_protocol: Protocol number of the newer version.
        new_packets: Names of packets added in the new version (have packet_id).
        new_types: Names of sub-types added in the new version (no packet_id).
        removed_packets: Names of packets removed in the new version.
        removed_types: Names of sub-types removed in the new version.
        changed_packets: Per-packet diffs keyed by packet name (have packet_id).
        changed_types: Per-type diffs keyed by type name (no packet_id).
    """

    old_protocol: int
    new_protocol: int
    new_packets: list[str] = []
    new_types: list[str] = []
    removed_packets: list[str] = []
    removed_types: list[str] = []
    changed_packets: dict[str, PacketChanges] = {}
    changed_types: dict[str, PacketChanges] = {}


def infer_direction(name: str) -> str | None:
    """Infer packet direction from its name.

    Args:
        name: Packet name (e.g. "ClientboundMapItemDataPacket").

    Returns:
        "clientbound", "serverbound", or None if direction cannot be inferred.
    """
    lower = name.lower()
    if lower.startswith("clientbound"):
        return "clientbound"
    if lower.startswith("serverbound"):
        return "serverbound"
    return None


def is_node_id_noise(field_name: str) -> bool:
    """Check if a field name is a DOT node ID artifact (e.g. "node_172").

    Args:
        field_name: A dotted field path component.

    Returns:
        True if any component of the path is a node_NNN pattern.
    """
    return any(_NODE_ID_RE.match(part) for part in field_name.split("."))
