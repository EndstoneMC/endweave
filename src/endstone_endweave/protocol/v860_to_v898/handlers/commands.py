"""Command and text packet handlers for v860 to v898."""

from dataclasses import dataclass

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    INT64_LE,
    INT_LE,
    STRING,
    USHORT_LE,
    UVAR_INT,
    UUID,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
)

_TEXT_KIND_MESSAGE_ONLY = 0
_TEXT_KIND_AUTHOR_AND_MESSAGE = 1
_TEXT_KIND_MESSAGE_AND_PARAMS = 2

_TEXT_MESSAGE_ONLY = {
    0: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    5: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    6: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    9: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    10: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
    11: ("raw", "tip", "systemMessage", "textObjectWhisper", "textObjectAnnouncement", "textObject"),
}
_TEXT_AUTHOR_AND_MESSAGE = {
    1: ("chat", "whisper", "announcement"),
    7: ("chat", "whisper", "announcement"),
    8: ("chat", "whisper", "announcement"),
}
_TEXT_MESSAGE_AND_PARAMS = {
    2: ("translate", "popup", "jukeboxPopup"),
    3: ("translate", "popup", "jukeboxPopup"),
    4: ("translate", "popup", "jukeboxPopup"),
}

_COMMAND_PERMISSION_LABELS = ["any", "gamedirectors", "admin", "host", "owner", "internal"]
_COMMAND_OUTPUT_LABELS = ["none", "lastoutput", "silent", "alloutput", "dataset"]


@dataclass
class _CommandEnum:
    name: str
    values: list[int]


@dataclass
class _Constraint:
    value_index: int
    enum_index: int
    constraints: list[int]


@dataclass
class _SubCommand:
    name: str
    values: list[tuple[int, int]]


@dataclass
class _Parameter:
    name: str
    symbol: int
    optional: bool
    options: int


@dataclass
class _Overload:
    chaining: bool
    parameters: list[_Parameter]


@dataclass
class _Command:
    name: str
    description: str
    flags: int
    permission: int
    alias_index: int
    subcommand_indices: list[int]
    overloads: list[_Overload]


def _read_index(wrapper: PacketWrapper, width: int) -> int:
    if width == 1:
        return wrapper.read(BYTE)
    if width == 2:
        return wrapper.read(USHORT_LE)
    return wrapper.read(INT_LE)


def _read_string_array(wrapper: PacketWrapper) -> list[str]:
    count = wrapper.read(UVAR_INT)
    return [wrapper.read(STRING) for _ in range(count)]


def _write_string_array(wrapper: PacketWrapper, values: list[str]) -> None:
    wrapper.write(UVAR_INT, len(values))
    for value in values:
        wrapper.write(STRING, value)


def _read_enum_array_old(wrapper: PacketWrapper, values_size: int) -> list[_CommandEnum]:
    count = wrapper.read(UVAR_INT)
    if values_size <= 0x100:
        width = 1
    elif values_size <= 0x10000:
        width = 2
    else:
        width = 4

    enums: list[_CommandEnum] = []
    for _ in range(count):
        name = wrapper.read(STRING)
        value_count = wrapper.read(UVAR_INT)
        enum_values = [_read_index(wrapper, width) for _ in range(value_count)]
        enums.append(_CommandEnum(name, enum_values))
    return enums


def _write_enum_array_new(wrapper: PacketWrapper, enums: list[_CommandEnum]) -> None:
    wrapper.write(UVAR_INT, len(enums))
    for enum_data in enums:
        wrapper.write(STRING, enum_data.name)
        wrapper.write(UVAR_INT, len(enum_data.values))
        for value in enum_data.values:
            wrapper.write(INT_LE, value)


def _read_subcommand_array_old(wrapper: PacketWrapper) -> list[_SubCommand]:
    count = wrapper.read(UVAR_INT)
    subcommands: list[_SubCommand] = []
    for _ in range(count):
        name = wrapper.read(STRING)
        value_count = wrapper.read(UVAR_INT)
        values = [(wrapper.read(USHORT_LE), wrapper.read(USHORT_LE)) for _ in range(value_count)]
        subcommands.append(_SubCommand(name, values))
    return subcommands


def _write_subcommand_array_new(wrapper: PacketWrapper, subcommands: list[_SubCommand]) -> None:
    wrapper.write(UVAR_INT, len(subcommands))
    for subcommand in subcommands:
        wrapper.write(STRING, subcommand.name)
        wrapper.write(UVAR_INT, len(subcommand.values))
        for first, second in subcommand.values:
            wrapper.write(UVAR_INT, first)
            wrapper.write(UVAR_INT, second)


def _read_parameter(wrapper: PacketWrapper) -> _Parameter:
    return _Parameter(
        name=wrapper.read(STRING),
        symbol=wrapper.read(INT_LE),
        optional=wrapper.read(BOOL),
        options=wrapper.read(BYTE),
    )


def _write_parameter(wrapper: PacketWrapper, parameter: _Parameter) -> None:
    wrapper.write(STRING, parameter.name)
    wrapper.write(INT_LE, parameter.symbol)
    wrapper.write(BOOL, parameter.optional)
    wrapper.write(BYTE, parameter.options)


def _read_command_array_old(wrapper: PacketWrapper) -> list[_Command]:
    count = wrapper.read(UVAR_INT)
    commands: list[_Command] = []
    for _ in range(count):
        name = wrapper.read(STRING)
        description = wrapper.read(STRING)
        flags = wrapper.read(USHORT_LE)
        permission = wrapper.read(BYTE)
        alias_index = wrapper.read(INT_LE)
        subcommand_count = wrapper.read(UVAR_INT)
        subcommand_indices = [wrapper.read(USHORT_LE) for _ in range(subcommand_count)]
        overload_count = wrapper.read(UVAR_INT)
        overloads: list[_Overload] = []
        for _ in range(overload_count):
            chaining = wrapper.read(BOOL)
            parameter_count = wrapper.read(UVAR_INT)
            parameters = [_read_parameter(wrapper) for _ in range(parameter_count)]
            overloads.append(_Overload(chaining=chaining, parameters=parameters))
        commands.append(
            _Command(
                name=name,
                description=description,
                flags=flags,
                permission=permission,
                alias_index=alias_index,
                subcommand_indices=subcommand_indices,
                overloads=overloads,
            )
        )
    return commands


def _write_command_array_new(wrapper: PacketWrapper, commands: list[_Command]) -> None:
    wrapper.write(UVAR_INT, len(commands))
    for command in commands:
        wrapper.write(STRING, command.name)
        wrapper.write(STRING, command.description)
        wrapper.write(USHORT_LE, command.flags)
        wrapper.write(STRING, _COMMAND_PERMISSION_LABELS[command.permission])
        wrapper.write(INT_LE, command.alias_index)
        wrapper.write(UVAR_INT, len(command.subcommand_indices))
        for subcommand_index in command.subcommand_indices:
            wrapper.write(INT_LE, subcommand_index)
        wrapper.write(UVAR_INT, len(command.overloads))
        for overload in command.overloads:
            wrapper.write(BOOL, overload.chaining)
            wrapper.write(UVAR_INT, len(overload.parameters))
            for parameter in overload.parameters:
                _write_parameter(wrapper, parameter)


def _read_soft_enums(wrapper: PacketWrapper) -> list[tuple[str, list[str]]]:
    count = wrapper.read(UVAR_INT)
    enums: list[tuple[str, list[str]]] = []
    for _ in range(count):
        name = wrapper.read(STRING)
        value_count = wrapper.read(UVAR_INT)
        values = [wrapper.read(STRING) for _ in range(value_count)]
        enums.append((name, values))
    return enums


def _write_soft_enums(wrapper: PacketWrapper, enums: list[tuple[str, list[str]]]) -> None:
    wrapper.write(UVAR_INT, len(enums))
    for name, values in enums:
        wrapper.write(STRING, name)
        wrapper.write(UVAR_INT, len(values))
        for value in values:
            wrapper.write(STRING, value)


def _read_constraints(wrapper: PacketWrapper) -> list[_Constraint]:
    count = wrapper.read(UVAR_INT)
    constraints: list[_Constraint] = []
    for _ in range(count):
        value_index = wrapper.read(INT_LE)
        enum_index = wrapper.read(INT_LE)
        item_count = wrapper.read(UVAR_INT)
        items = [wrapper.read(BYTE) for _ in range(item_count)]
        constraints.append(_Constraint(value_index=value_index, enum_index=enum_index, constraints=items))
    return constraints


def _write_constraints(wrapper: PacketWrapper, constraints: list[_Constraint]) -> None:
    wrapper.write(UVAR_INT, len(constraints))
    for constraint in constraints:
        wrapper.write(INT_LE, constraint.value_index)
        wrapper.write(INT_LE, constraint.enum_index)
        wrapper.write(UVAR_INT, len(constraint.constraints))
        for value in constraint.constraints:
            wrapper.write(BYTE, value)


def _read_command_origin_old(wrapper: PacketWrapper) -> tuple[int, bytes, str, int]:
    origin = wrapper.read(UVAR_INT)
    uuid = wrapper.read(UUID)
    request_id = wrapper.read(STRING)
    player_id = -1
    if origin in (4, 5):
        player_id = wrapper.read(VAR_INT64)
    return origin, uuid, request_id, player_id


def _write_command_origin_old(wrapper: PacketWrapper, uuid: bytes, request_id: str) -> None:
    wrapper.write(UVAR_INT, 0)
    wrapper.write(UUID, uuid)
    wrapper.write(STRING, request_id)


def _read_command_origin_new(wrapper: PacketWrapper) -> tuple[bytes, str, int]:
    wrapper.read(STRING)
    uuid = wrapper.read(UUID)
    request_id = wrapper.read(STRING)
    player_id = wrapper.read(INT64_LE)
    return uuid, request_id, player_id


def _write_command_origin_new(wrapper: PacketWrapper, uuid: bytes, request_id: str, player_id: int) -> None:
    wrapper.write(STRING, "player")
    wrapper.write(UUID, uuid)
    wrapper.write(STRING, request_id)
    wrapper.write(INT64_LE, player_id)


def rewrite_available_commands(wrapper: PacketWrapper) -> None:
    """Rewrite AvailableCommands from the v860 layout to v898.

    Args:
        wrapper: Packet wrapper for AvailableCommands.
    """
    enum_values = _read_string_array(wrapper)
    subcommand_values = _read_string_array(wrapper)
    postfixes = _read_string_array(wrapper)
    enums = _read_enum_array_old(wrapper, len(enum_values))
    subcommands = _read_subcommand_array_old(wrapper)
    commands = _read_command_array_old(wrapper)
    soft_enums = _read_soft_enums(wrapper)
    constraints = _read_constraints(wrapper)

    _write_string_array(wrapper, enum_values)
    _write_string_array(wrapper, subcommand_values)
    _write_string_array(wrapper, postfixes)
    _write_enum_array_new(wrapper, enums)
    _write_subcommand_array_new(wrapper, subcommands)
    _write_command_array_new(wrapper, commands)
    _write_soft_enums(wrapper, soft_enums)
    _write_constraints(wrapper, constraints)


def rewrite_command_request(wrapper: PacketWrapper) -> None:
    """Rewrite CommandRequest back to the v860 command origin and version.

    Args:
        wrapper: Packet wrapper for CommandRequest.
    """
    wrapper.passthrough(STRING)
    uuid, request_id, _ = _read_command_origin_new(wrapper)
    _write_command_origin_old(wrapper, uuid, request_id)
    wrapper.passthrough(BOOL)
    wrapper.read(STRING)
    wrapper.write(VAR_INT, 48)


def rewrite_command_output(wrapper: PacketWrapper) -> None:
    """Rewrite CommandOutput into the v898 layout.

    Args:
        wrapper: Packet wrapper for CommandOutput.
    """
    _, uuid, request_id, player_id = _read_command_origin_old(wrapper)
    _write_command_origin_new(wrapper, uuid, request_id, player_id)

    output_type = wrapper.read(BYTE)
    wrapper.write(STRING, _COMMAND_OUTPUT_LABELS[output_type])
    wrapper.write(INT_LE, wrapper.read(UVAR_INT))

    message_count = wrapper.read(UVAR_INT)
    wrapper.write(UVAR_INT, message_count)
    for _ in range(message_count):
        internal = wrapper.read(BOOL)
        message_id = wrapper.read(STRING)
        parameter_count = wrapper.read(UVAR_INT)
        parameters = [wrapper.read(STRING) for _ in range(parameter_count)]

        wrapper.write(STRING, message_id)
        wrapper.write(BOOL, internal)
        wrapper.write(UVAR_INT, len(parameters))
        for parameter in parameters:
            wrapper.write(STRING, parameter)

    has_data = output_type == 4
    wrapper.write(BOOL, has_data)
    if has_data:
        wrapper.passthrough(STRING)


def rewrite_text_clientbound(wrapper: PacketWrapper) -> None:
    """Rewrite Text from the v860 wire format to the v898 format.

    Args:
        wrapper: Packet wrapper for Text.
    """
    text_type = wrapper.read(BYTE)
    needs_translation = wrapper.read(BOOL)

    wrapper.write(BOOL, needs_translation)
    if text_type in _TEXT_MESSAGE_ONLY:
        wrapper.write(BYTE, _TEXT_KIND_MESSAGE_ONLY)
        for label in _TEXT_MESSAGE_ONLY[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
    elif text_type in _TEXT_AUTHOR_AND_MESSAGE:
        wrapper.write(BYTE, _TEXT_KIND_AUTHOR_AND_MESSAGE)
        for label in _TEXT_AUTHOR_AND_MESSAGE[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
    elif text_type in _TEXT_MESSAGE_AND_PARAMS:
        wrapper.write(BYTE, _TEXT_KIND_MESSAGE_AND_PARAMS)
        for label in _TEXT_MESSAGE_AND_PARAMS[text_type]:
            wrapper.write(STRING, label)
        wrapper.write(BYTE, text_type)
        wrapper.passthrough(STRING)
        parameter_count = wrapper.passthrough(UVAR_INT)
        for _ in range(parameter_count):
            wrapper.passthrough(STRING)
    else:
        raise ValueError(f"Unknown text type: {text_type}")

    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    filtered_message = wrapper.read(STRING)
    wrapper.write(BOOL, filtered_message != "")
    if filtered_message != "":
        wrapper.write(STRING, filtered_message)


def rewrite_text_serverbound(wrapper: PacketWrapper) -> None:
    """Rewrite Text from the v898 wire format to the v860 format.

    Args:
        wrapper: Packet wrapper for Text.
    """
    needs_translation = wrapper.read(BOOL)
    kind = wrapper.read(BYTE)

    if kind == _TEXT_KIND_MESSAGE_ONLY:
        for _ in range(6):
            wrapper.read(STRING)
        text_type = wrapper.read(BYTE)
        wrapper.write(BYTE, text_type)
        wrapper.write(BOOL, needs_translation)
        wrapper.passthrough(STRING)
    elif kind == _TEXT_KIND_AUTHOR_AND_MESSAGE:
        for _ in range(3):
            wrapper.read(STRING)
        text_type = wrapper.read(BYTE)
        wrapper.write(BYTE, text_type)
        wrapper.write(BOOL, needs_translation)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
    elif kind == _TEXT_KIND_MESSAGE_AND_PARAMS:
        for _ in range(3):
            wrapper.read(STRING)
        text_type = wrapper.read(BYTE)
        wrapper.write(BYTE, text_type)
        wrapper.write(BOOL, needs_translation)
        wrapper.passthrough(STRING)
        parameter_count = wrapper.passthrough(UVAR_INT)
        for _ in range(parameter_count):
            wrapper.passthrough(STRING)
    else:
        raise ValueError(f"Unknown text kind: {kind}")

    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    filtered_message = ""
    if wrapper.read(BOOL):
        filtered_message = wrapper.read(STRING)
    wrapper.write(STRING, filtered_message)
