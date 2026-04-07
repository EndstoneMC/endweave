"""Command packet handlers for v898 to v860."""

from ....codec import (
    BOOL,
    BYTE,
    COMMAND_CONSTRAINT,
    COMMAND_DEFINITION_V860,
    COMMAND_DEFINITION_V898,
    COMMAND_ENUM_V898,
    COMMAND_ORIGIN_V860,
    COMMAND_ORIGIN_V898,
    COMMAND_SUBCOMMAND_V860,
    COMMAND_SUBCOMMAND_V898,
    INT_LE,
    SOFT_ENUM,
    STRING,
    UVAR_INT,
    VAR_INT,
    ArrayType,
    CommandOutputType,
    PacketWrapper,
    label_to_enum,
    make_command_enum_v860,
)

_OUTPUT_TYPE_BY_LABEL = label_to_enum(CommandOutputType)


def rewrite_available_commands(wrapper: PacketWrapper) -> None:
    """Rewrite AvailableCommands from the v898 layout to v860.

    Args:
        wrapper: Packet wrapper for AvailableCommands.
    """
    enum_values = wrapper.passthrough(ArrayType(STRING))  # Enum Values
    wrapper.passthrough(ArrayType(STRING))  # Subcommand Values
    wrapper.passthrough(ArrayType(STRING))  # Postfixes
    wrapper.map(ArrayType(COMMAND_ENUM_V898), ArrayType(make_command_enum_v860(len(enum_values))))  # Enums
    wrapper.map(ArrayType(COMMAND_SUBCOMMAND_V898), ArrayType(COMMAND_SUBCOMMAND_V860))  # Subcommands
    wrapper.map(ArrayType(COMMAND_DEFINITION_V898), ArrayType(COMMAND_DEFINITION_V860))  # Commands
    wrapper.passthrough(ArrayType(SOFT_ENUM))  # Soft Enums
    wrapper.passthrough(ArrayType(COMMAND_CONSTRAINT))  # Constraints


def rewrite_command_request(wrapper: PacketWrapper) -> None:
    """Rewrite CommandRequest into the v898 command origin and version.

    Args:
        wrapper: Packet wrapper for CommandRequest.
    """
    wrapper.passthrough(STRING)
    wrapper.map(COMMAND_ORIGIN_V860, COMMAND_ORIGIN_V898)  # Origin
    wrapper.passthrough(BOOL)
    wrapper.read(VAR_INT)
    wrapper.write(STRING, "latest")


def rewrite_command_output(wrapper: PacketWrapper) -> None:
    """Rewrite CommandOutput into the v860 layout.

    Args:
        wrapper: Packet wrapper for CommandOutput.
    """
    wrapper.map(COMMAND_ORIGIN_V898, COMMAND_ORIGIN_V860)  # Origin Data

    output_type = _OUTPUT_TYPE_BY_LABEL[wrapper.read(STRING)]
    wrapper.write(BYTE, output_type)
    wrapper.map(INT_LE, UVAR_INT)  # Success Count

    message_count = wrapper.passthrough(UVAR_INT)  # Output Messages
    for _ in range(message_count):
        message_id = wrapper.read(STRING)
        internal = wrapper.read(BOOL)
        parameter_count = wrapper.read(UVAR_INT)
        parameters = [wrapper.read(STRING) for _ in range(parameter_count)]

        wrapper.write(BOOL, internal)
        wrapper.write(STRING, message_id)
        wrapper.write(UVAR_INT, len(parameters))
        for parameter in parameters:
            wrapper.write(STRING, parameter)

    wrapper.read(BOOL)


__all__ = [
    "rewrite_available_commands",
    "rewrite_command_output",
    "rewrite_command_request",
]
