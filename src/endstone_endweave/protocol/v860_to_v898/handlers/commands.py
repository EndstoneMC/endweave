"""Command packet handlers for v860 to v898."""

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
    CommandOrigin,
    CommandOutputType,
    CurrentCmdVersion,
    PacketWrapper,
    enum_to_label,
    make_command_enum_v860,
)

_LABEL_BY_OUTPUT_TYPE = enum_to_label(CommandOutputType)


def rewrite_available_commands(wrapper: PacketWrapper) -> None:
    """Rewrite AvailableCommands from the v860 layout to v898.

    Args:
        wrapper: Packet wrapper for AvailableCommands.
    """
    enum_values = wrapper.passthrough(ArrayType(STRING))  # Enum Values
    wrapper.passthrough(ArrayType(STRING))  # Subcommand Values
    wrapper.passthrough(ArrayType(STRING))  # Postfixes
    wrapper.map(ArrayType(make_command_enum_v860(len(enum_values))), ArrayType(COMMAND_ENUM_V898))  # Enums
    wrapper.map(ArrayType(COMMAND_SUBCOMMAND_V860), ArrayType(COMMAND_SUBCOMMAND_V898))  # Subcommands
    wrapper.map(ArrayType(COMMAND_DEFINITION_V860), ArrayType(COMMAND_DEFINITION_V898))  # Commands
    wrapper.passthrough(ArrayType(SOFT_ENUM))  # Soft Enums
    wrapper.passthrough(ArrayType(COMMAND_CONSTRAINT))  # Constraints


def rewrite_command_request(wrapper: PacketWrapper) -> None:
    """Rewrite CommandRequest back to the v860 command origin and version.

    Args:
        wrapper: Packet wrapper for CommandRequest.
    """
    wrapper.passthrough(STRING)  # Command
    origin = wrapper.read(COMMAND_ORIGIN_V898)  # Origin
    wrapper.write(COMMAND_ORIGIN_V860, CommandOrigin(0, origin.uuid, origin.request_id, -1))  # Command Origin
    wrapper.passthrough(BOOL)  # Is Internal Source?
    wrapper.read(STRING)  # Version
    wrapper.write(VAR_INT, CurrentCmdVersion.CLONE_EXTRA_BLOCK_FILTER_FIX)  # Version


def rewrite_command_output(wrapper: PacketWrapper) -> None:
    """Rewrite CommandOutput into the v898 layout.

    Args:
        wrapper: Packet wrapper for CommandOutput.
    """
    wrapper.map(COMMAND_ORIGIN_V860, COMMAND_ORIGIN_V898)  # Origin Data

    output_type = wrapper.read(BYTE)  # Output Type
    wrapper.write(STRING, _LABEL_BY_OUTPUT_TYPE[output_type])  # Output Type
    wrapper.map(UVAR_INT, INT_LE)  # Success Count

    message_count = wrapper.passthrough(UVAR_INT)  # Output Messages
    for _ in range(message_count):
        internal = wrapper.read(BOOL)  # Successful?
        message_id = wrapper.read(STRING)  # Message ID
        parameter_count = wrapper.read(UVAR_INT)  # Parameters
        parameters = [wrapper.read(STRING) for _ in range(parameter_count)]

        wrapper.write(STRING, message_id)  # Message ID
        wrapper.write(BOOL, internal)  # Successful?
        wrapper.write(UVAR_INT, len(parameters))  # Parameters
        for parameter in parameters:
            wrapper.write(STRING, parameter)

    wrapper.write(BOOL, output_type == CommandOutputType.DATA_SET)  # Data Set
