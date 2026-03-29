"""Pydantic models for the protocol docs tooling pipeline.

Shared data structures used by parse.py and diff.py.
"""

import re

from pydantic import BaseModel
from pydantic import Field as PydanticField


class Field(BaseModel):
    """A single field in a packet or type definition.

    Attributes:
        name: Field name (e.g. "Block Position", "X").
        type: Type name (e.g. "NetworkBlockPosition", "varint32").
        list: True if this field is a list/array with a count prefix.
        fields: Nested sub-fields in serialization order.
        attributes: Internal DOT bitmask, excluded from serialized output.
    """

    name: str
    type: str = ""
    fields: list["Field"] = []
    list: bool = False
    optional: bool = False
    attributes: int = PydanticField(default=0, exclude=True)


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


class FieldChange(BaseModel):
    """A field that was added or removed between protocol versions."""

    name: str
    type: str = ""


class TypeChange(BaseModel):
    """A field type that changed between protocol versions.

    When old == new, the type's internal structure changed.
    The added/removed lists describe what changed inside.
    """

    old: str
    new: str
    added: list[str] = []
    removed: list[str] = []


class PacketChanges(BaseModel):
    """Per-packet diff between two protocol versions.

    Attributes:
        packet_id: Numeric packet ID (from old version), None for sub-types.
        direction: Packet direction, None if unknown.
        added_fields: Fields only in the new version (with types).
        removed_fields: Fields only in the old version (with types).
        type_changes: Fields whose type changed, keyed by dotted path.
    """

    packet_id: int | None = None
    direction: str | None = None
    added_fields: list[FieldChange] = []
    removed_fields: list[FieldChange] = []
    type_changes: dict[str, TypeChange] = {}


class EnumEntry(BaseModel):
    """A single enum value added in a protocol version.

    Attributes:
        name: Enum value name (e.g. "Trumpet", "PauseGrowth").
        id: Numeric value if known, None otherwise.
    """

    name: str
    id: int | None = None


class EnumChange(BaseModel):
    """Changes to a single enum between protocol versions.

    Attributes:
        added: New enum values with their IDs.
        displaced: Existing values that shifted due to insertions above them.
        removed: Values removed entirely.
        changed: Values whose definition changed (e.g. flag composition).
    """

    added: list[EnumEntry] = []
    displaced: list[str] = []
    removed: list[str] = []
    changed: list[str] = []


class ChangelogEntry(BaseModel):
    """A single raw changelog entry from the protocol docs.

    Attributes:
        protocol: Protocol version number for this change.
        description: Human-readable description of what changed.
    """

    protocol: int
    description: str


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
        renamed_packets: Maps old packet name to new packet name for renames.
        changed_packets: Per-packet diffs keyed by packet name (have packet_id).
        changed_types: Per-type diffs keyed by type name (no packet_id).
        enum_changes: Enum modifications between versions, keyed by enum name.
        changelog: Raw per-protocol-step changelog entries.
    """

    old_protocol: int
    new_protocol: int
    new_packets: list[str] = []
    new_types: list[str] = []
    removed_packets: list[str] = []
    removed_types: list[str] = []
    renamed_packets: dict[str, str] = {}
    changed_packets: dict[str, PacketChanges] = {}
    changed_types: dict[str, PacketChanges] = {}
    enum_changes: dict[str, EnumChange] = {}
    changelog: list[ChangelogEntry] = []


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
