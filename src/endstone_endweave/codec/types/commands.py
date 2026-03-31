"""Command-related compound types for AvailableCommandsPacket serialization."""

from dataclasses import dataclass

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.types.enums import (
    CommandOriginType,
    CommandPermissionLevel,
    enum_to_label,
    label_to_enum,
)
from endstone_endweave.codec.types.primitives import (
    BOOL,
    BYTE,
    INT64_LE,
    INT_LE,
    STRING,
    USHORT_LE,
    UUID,
    UVAR_INT,
    VAR_INT64,
    Type,
)
from endstone_endweave.codec.writer import PacketWriter

# ---------------------------------------------------------------------------
# Shared dataclasses
# ---------------------------------------------------------------------------


@dataclass
class CommandEnum:
    """A command enum with name and value indices."""

    name: str
    values: list[int]


@dataclass
class CommandSubcommand:
    """A command subcommand with name and value pairs."""

    name: str
    values: list[tuple[int, int]]


@dataclass
class CommandParameter:
    """A single command parameter descriptor."""

    name: str
    symbol: int
    optional: bool
    options: int


@dataclass
class CommandOverload:
    """A command overload with chaining flag and parameter list."""

    chaining: bool
    parameters: list[CommandParameter]


@dataclass
class CommandDefinition:
    """A full command definition from AvailableCommandsPacket."""

    name: str
    description: str
    flags: int
    permission: int
    alias_index: int
    subcommand_indices: list[int]
    overloads: list[CommandOverload]


@dataclass
class CommandConstraint:
    """An enum value constraint."""

    value_index: int
    enum_index: int
    constraints: list[int]


@dataclass
class SoftEnum:
    """A dynamic (soft) enum with name and string values."""

    name: str
    values: list[str]


@dataclass
class CommandOrigin:
    """Command origin data from CommandRequestPacket / CommandOutputPacket."""

    origin_type: int
    uuid: bytes
    request_id: str
    player_id: int


# ---------------------------------------------------------------------------
# CommandParameter (same wire format in v860 and v898)
# ---------------------------------------------------------------------------


class _CommandParameterType(Type["CommandParameter"]):
    def read(self, reader: PacketReader) -> CommandParameter:
        return CommandParameter(
            name=STRING.read(reader),
            symbol=INT_LE.read(reader),
            optional=BOOL.read(reader),
            options=BYTE.read(reader),
        )

    def write(self, writer: PacketWriter, value: CommandParameter) -> None:
        STRING.write(writer, value.name)
        INT_LE.write(writer, value.symbol)
        BOOL.write(writer, value.optional)
        BYTE.write(writer, value.options)


COMMAND_PARAMETER = _CommandParameterType()

# ---------------------------------------------------------------------------
# CommandOverload (same wire format in v860 and v898)
# ---------------------------------------------------------------------------


class _CommandOverloadType(Type["CommandOverload"]):
    def read(self, reader: PacketReader) -> CommandOverload:
        chaining = BOOL.read(reader)
        count = UVAR_INT.read(reader)
        parameters = [COMMAND_PARAMETER.read(reader) for _ in range(count)]
        return CommandOverload(chaining=chaining, parameters=parameters)

    def write(self, writer: PacketWriter, value: CommandOverload) -> None:
        BOOL.write(writer, value.chaining)
        UVAR_INT.write(writer, len(value.parameters))
        for param in value.parameters:
            COMMAND_PARAMETER.write(writer, param)


COMMAND_OVERLOAD = _CommandOverloadType()

# ---------------------------------------------------------------------------
# CommandEnum -- v860 uses variable-width indices, v898 uses INT_LE
# ---------------------------------------------------------------------------


class _CommandEnumV898Type(Type["CommandEnum"]):
    def read(self, reader: PacketReader) -> CommandEnum:
        name = STRING.read(reader)
        count = UVAR_INT.read(reader)
        values = [INT_LE.read(reader) for _ in range(count)]
        return CommandEnum(name=name, values=values)

    def write(self, writer: PacketWriter, value: CommandEnum) -> None:
        STRING.write(writer, value.name)
        UVAR_INT.write(writer, len(value.values))
        for v in value.values:
            INT_LE.write(writer, v)


COMMAND_ENUM_V898 = _CommandEnumV898Type()


def make_command_enum_v860(values_size: int) -> Type["CommandEnum"]:
    """Create a v860 CommandEnum Type with index width based on values_size.

    Args:
        values_size: Total number of enum values, determines index width
            (BYTE if <= 256, USHORT_LE if <= 65536, INT_LE otherwise).

    Returns:
        A Type instance for reading/writing v860 CommandEnum entries.
    """
    index_type: Type[int]
    if values_size <= 0x100:
        index_type = BYTE
    elif values_size <= 0x10000:
        index_type = USHORT_LE
    else:
        index_type = INT_LE

    class _CommandEnumV860Type(Type["CommandEnum"]):
        def read(self, reader: PacketReader) -> CommandEnum:
            name = STRING.read(reader)
            count = UVAR_INT.read(reader)
            values = [index_type.read(reader) for _ in range(count)]
            return CommandEnum(name=name, values=values)

        def write(self, writer: PacketWriter, value: CommandEnum) -> None:
            STRING.write(writer, value.name)
            UVAR_INT.write(writer, len(value.values))
            for v in value.values:
                index_type.write(writer, v)

    return _CommandEnumV860Type()


# ---------------------------------------------------------------------------
# CommandSubcommand -- v860 uses USHORT_LE pairs, v898 uses UVAR_INT pairs
# ---------------------------------------------------------------------------


class _CommandSubcommandV860Type(Type["CommandSubcommand"]):
    def read(self, reader: PacketReader) -> CommandSubcommand:
        name = STRING.read(reader)
        count = UVAR_INT.read(reader)
        values = [(USHORT_LE.read(reader), USHORT_LE.read(reader)) for _ in range(count)]
        return CommandSubcommand(name=name, values=values)

    def write(self, writer: PacketWriter, value: CommandSubcommand) -> None:
        STRING.write(writer, value.name)
        UVAR_INT.write(writer, len(value.values))
        for first, second in value.values:
            USHORT_LE.write(writer, first)
            USHORT_LE.write(writer, second)


COMMAND_SUBCOMMAND_V860 = _CommandSubcommandV860Type()


class _CommandSubcommandV898Type(Type["CommandSubcommand"]):
    def read(self, reader: PacketReader) -> CommandSubcommand:
        name = STRING.read(reader)
        count = UVAR_INT.read(reader)
        values = [(UVAR_INT.read(reader), UVAR_INT.read(reader)) for _ in range(count)]
        return CommandSubcommand(name=name, values=values)

    def write(self, writer: PacketWriter, value: CommandSubcommand) -> None:
        STRING.write(writer, value.name)
        UVAR_INT.write(writer, len(value.values))
        for first, second in value.values:
            UVAR_INT.write(writer, first)
            UVAR_INT.write(writer, second)


COMMAND_SUBCOMMAND_V898 = _CommandSubcommandV898Type()

# ---------------------------------------------------------------------------
# CommandDefinition -- v860 uses BYTE permission + USHORT subcommand indices,
# v898 uses STRING permission label + INT_LE subcommand indices
# ---------------------------------------------------------------------------


class _CommandDefinitionV860Type(Type["CommandDefinition"]):
    def read(self, reader: PacketReader) -> CommandDefinition:
        name = STRING.read(reader)
        description = STRING.read(reader)
        flags = USHORT_LE.read(reader)
        permission = BYTE.read(reader)
        alias_index = INT_LE.read(reader)
        sub_count = UVAR_INT.read(reader)
        subcommand_indices = [USHORT_LE.read(reader) for _ in range(sub_count)]
        overload_count = UVAR_INT.read(reader)
        overloads = [COMMAND_OVERLOAD.read(reader) for _ in range(overload_count)]
        return CommandDefinition(
            name=name,
            description=description,
            flags=flags,
            permission=permission,
            alias_index=alias_index,
            subcommand_indices=subcommand_indices,
            overloads=overloads,
        )

    def write(self, writer: PacketWriter, value: CommandDefinition) -> None:
        STRING.write(writer, value.name)
        STRING.write(writer, value.description)
        USHORT_LE.write(writer, value.flags)
        BYTE.write(writer, value.permission)
        INT_LE.write(writer, value.alias_index)
        UVAR_INT.write(writer, len(value.subcommand_indices))
        for idx in value.subcommand_indices:
            USHORT_LE.write(writer, idx)
        UVAR_INT.write(writer, len(value.overloads))
        for overload in value.overloads:
            COMMAND_OVERLOAD.write(writer, overload)


COMMAND_DEFINITION_V860 = _CommandDefinitionV860Type()


class _CommandDefinitionV898Type(Type["CommandDefinition"]):
    _by_label = label_to_enum(CommandPermissionLevel)
    _by_value = enum_to_label(CommandPermissionLevel)

    def read(self, reader: PacketReader) -> CommandDefinition:
        name = STRING.read(reader)
        description = STRING.read(reader)
        flags = USHORT_LE.read(reader)
        permission = self._by_label[STRING.read(reader)]
        alias_index = INT_LE.read(reader)
        sub_count = UVAR_INT.read(reader)
        subcommand_indices = [INT_LE.read(reader) for _ in range(sub_count)]
        overload_count = UVAR_INT.read(reader)
        overloads = [COMMAND_OVERLOAD.read(reader) for _ in range(overload_count)]
        return CommandDefinition(
            name=name,
            description=description,
            flags=flags,
            permission=permission,
            alias_index=alias_index,
            subcommand_indices=subcommand_indices,
            overloads=overloads,
        )

    def write(self, writer: PacketWriter, value: CommandDefinition) -> None:
        STRING.write(writer, value.name)
        STRING.write(writer, value.description)
        USHORT_LE.write(writer, value.flags)
        STRING.write(writer, self._by_value[value.permission])
        INT_LE.write(writer, value.alias_index)
        UVAR_INT.write(writer, len(value.subcommand_indices))
        for idx in value.subcommand_indices:
            INT_LE.write(writer, idx)
        UVAR_INT.write(writer, len(value.overloads))
        for overload in value.overloads:
            COMMAND_OVERLOAD.write(writer, overload)


COMMAND_DEFINITION_V898 = _CommandDefinitionV898Type()

# ---------------------------------------------------------------------------
# SoftEnum (same wire format in v860 and v898)
# ---------------------------------------------------------------------------


class _SoftEnumType(Type["SoftEnum"]):
    def read(self, reader: PacketReader) -> SoftEnum:
        name = STRING.read(reader)
        count = UVAR_INT.read(reader)
        values = [STRING.read(reader) for _ in range(count)]
        return SoftEnum(name=name, values=values)

    def write(self, writer: PacketWriter, value: SoftEnum) -> None:
        STRING.write(writer, value.name)
        UVAR_INT.write(writer, len(value.values))
        for v in value.values:
            STRING.write(writer, v)


SOFT_ENUM = _SoftEnumType()

# ---------------------------------------------------------------------------
# CommandConstraint (same wire format in v860 and v898)
# ---------------------------------------------------------------------------


class _CommandConstraintType(Type["CommandConstraint"]):
    def read(self, reader: PacketReader) -> CommandConstraint:
        value_index = INT_LE.read(reader)
        enum_index = INT_LE.read(reader)
        count = UVAR_INT.read(reader)
        constraints = [BYTE.read(reader) for _ in range(count)]
        return CommandConstraint(value_index=value_index, enum_index=enum_index, constraints=constraints)

    def write(self, writer: PacketWriter, value: CommandConstraint) -> None:
        INT_LE.write(writer, value.value_index)
        INT_LE.write(writer, value.enum_index)
        UVAR_INT.write(writer, len(value.constraints))
        for c in value.constraints:
            BYTE.write(writer, c)


COMMAND_CONSTRAINT = _CommandConstraintType()

# ---------------------------------------------------------------------------
# CommandOrigin -- v860 uses UVAR_INT type, v898 uses STRING type
# ---------------------------------------------------------------------------


class _CommandOriginV860Type(Type["CommandOrigin"]):
    def read(self, reader: PacketReader) -> CommandOrigin:
        origin_type = UVAR_INT.read(reader)
        uuid = UUID.read(reader)
        request_id = STRING.read(reader)
        player_id = -1
        if origin_type in (CommandOriginType.TEST, CommandOriginType.AUTOMATION_PLAYER):
            player_id = VAR_INT64.read(reader)
        return CommandOrigin(origin_type=origin_type, uuid=uuid, request_id=request_id, player_id=player_id)

    def write(self, writer: PacketWriter, value: CommandOrigin) -> None:
        UVAR_INT.write(writer, value.origin_type)
        UUID.write(writer, value.uuid)
        STRING.write(writer, value.request_id)
        if value.origin_type in (CommandOriginType.TEST, CommandOriginType.AUTOMATION_PLAYER):
            VAR_INT64.write(writer, value.player_id)


COMMAND_ORIGIN_V860 = _CommandOriginV860Type()


class _CommandOriginV898Type(Type["CommandOrigin"]):
    _by_label = label_to_enum(CommandOriginType)
    _by_value = enum_to_label(CommandOriginType)

    def read(self, reader: PacketReader) -> CommandOrigin:
        origin_label = STRING.read(reader)
        uuid = UUID.read(reader)
        request_id = STRING.read(reader)
        player_id = INT64_LE.read(reader)
        origin_type = self._by_label.get(origin_label, 0)
        return CommandOrigin(
            origin_type=origin_type,
            uuid=uuid,
            request_id=request_id,
            player_id=player_id,
        )

    def write(self, writer: PacketWriter, value: CommandOrigin) -> None:
        STRING.write(writer, self._by_value.get(value.origin_type, "player"))
        UUID.write(writer, value.uuid)
        STRING.write(writer, value.request_id)
        INT64_LE.write(writer, value.player_id)


COMMAND_ORIGIN_V898 = _CommandOriginV898Type()
