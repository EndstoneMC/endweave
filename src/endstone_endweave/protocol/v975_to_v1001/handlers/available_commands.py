"""Clientbound AvailableCommands(76) handler for the v975 -> v1001 delta.

What changed at v1001 (MC 1.26.30 "Chaos Cubed")
-------------------------------------------------
The AvailableCommandsPacket *container* layout did NOT change between protocol
975 and 1001: the enum-value / subcommand-value / postfix string arrays, the
CommandEnum / CommandSubcommand / CommandDefinition / SoftEnum / CommandConstraint
encodings, and the CommandParameter wire shape (string Name, uint32 LE Type, bool
Optional, byte Options) are all byte-identical to the v898+ format the existing
``COMMAND_*_V898`` codecs already model.

What DID change is the integer VALUE space of ``CommandParameter.Type``. The low
bits of that uint32 are a ``CommandArgType`` parser id; the high bits are flags
(``CommandArgValid 0x100000``, ``CommandArgEnum 0x200000``,
``CommandArgSoftEnum 0x4000000``, ``CommandArgSuffixed 0x1000000``). Mojang's
command registry assigns those parser ids by runtime enumeration order, and
1.26.30 inserted new parser symbols, shifting the ids of some later parsers. A
v944/v975 server emits the OLD ids; an untranslated v1001 client decodes them
against its NEW dense table and mis-types parameters, corrupting the command
tree mid-parse and dropping the client during the join burst (AvailableCommands
is large and is sent unconditionally at join).

Ground truth (verified by diffing the raw ``minecraft/protocol/command.go`` at
gophertunnel v1.56.2 [=protocol 975] vs v1.57.0 [=protocol 1001], pairing the
named ``CommandArgType*`` constants by NAME across both tags). v1.56.2 uses
sparse explicit values; v1.57.0 switched to a dense ``iota+1`` block with ~70
new intermediate parser names inserted. Of the 19 parser ids a vanilla BDS
command tree actually emits, exactly THREE shifted:

    CommandArgTypeFloat        3 -> 2
    CommandArgTypeValue        4 -> 3
    CommandArgTypeBlockStates  83 -> 84

Every other named parser id (Int 1, WildcardInt 5, Operator 6, CompareOperator
7, Target 8, WildcardTarget 10, Filepath 17, IntegerRange 23, EquipmentSlots 47,
String 56, BlockPosition 64, Position 65, Message 67, RawText 70, JSON 74,
Command 87) is UNCHANGED across the two tags, so a complete-named-set old->new
table is the identity except for those three. ``CommandParameter.Marshal`` writes
``Type`` as a raw ``r.Uint32`` with no remapping at either tag, so the value on
the wire IS the raw parser id -- this handler performs the only remap that exists.

Why both upstream diff investigations were wrong about the table
---------------------------------------------------------------
Reading only the v1.57.0 dense block in isolation makes it look like nearly every
id shifted (Investigation 1) or many shifted downward (Investigation 2). Both
mis-paired the constants. Pairing by NAME across BOTH tags is the only correct
method, and it yields just the three deltas above. Remapping more than these
three would CORRUPT correctly-valued parameters, so the table is deliberately
minimal.

Direction / placement
---------------------
This is CLIENTBOUND and lives in the v975->v1001 layer because the id space is
target-version specific: a real v975 (1.26.20) client still expects the OLD ids,
so the v944_to_v975 layer must NOT touch them. Only a v1001-terminal client gets
this handler in its chain.

The flag bits are preserved exactly; only the masked-off arg-type id is
translated, and only when the value is a plain parser id (i.e. NOT an enum /
soft-enum / postfix reference, whose low bits are an index into the enum tables,
not a parser id).
"""

from endstone_endweave.codec import (
    COMMAND_CONSTRAINT,
    COMMAND_DEFINITION_V898,
    COMMAND_ENUM_V898,
    COMMAND_SUBCOMMAND_V898,
    SOFT_ENUM,
    STRING,
    ArrayType,
    CommandDefinition,
    PacketWrapper,
)

# Flag bits occupying the high portion of CommandParameter.Type. A parameter
# whose Type carries CommandArgEnum / CommandArgSoftEnum references an entry in
# the enum / soft-enum tables and its low bits are an INDEX, not a parser id, so
# it must NOT be remapped.
_COMMAND_ARG_VALID = 0x100000
_COMMAND_ARG_ENUM = 0x200000
_COMMAND_ARG_SOFT_ENUM = 0x4000000
_COMMAND_ARG_SUFFIXED = 0x1000000

# Low-bits mask for the arg-type parser id (everything below the flag region).
_ARG_TYPE_MASK = 0xFFFF

# OLD (v975) parser id -> NEW (v1001) parser id. Identity except for the three
# ids that shifted when v1.57.0 inserted new parser symbols. Verified by pairing
# every named CommandArgType* constant by NAME across gophertunnel v1.56.2 and
# v1.57.0; all unlisted ids are unchanged and pass through untouched.
_ARG_TYPE_OLD_TO_NEW: dict[int, int] = {
    3: 2,    # CommandArgTypeFloat        3 -> 2
    4: 3,    # CommandArgTypeValue        4 -> 3
    83: 84,  # CommandArgTypeBlockStates  83 -> 84
}


def _remap_arg_type(symbol: int) -> int:
    """Translate one CommandParameter.Type from the OLD (v975) to NEW (v1001) id.

    Preserves the flag bits exactly. Only remaps the low arg-type id, and only
    for plain parser parameters: a parameter that references an enum or soft enum
    carries an index in its low bits (not a parser id) and is left untouched.

    Args:
        symbol: The raw uint32 CommandParameter.Type from the v975 wire.

    Returns:
        The uint32 Type with its low arg-type id remapped to the v1001 value
        (flags unchanged). Returns the input unchanged when it is an enum /
        soft-enum reference or when its arg-type id did not shift at v1001.
    """
    # Enum / soft-enum parameters index the enum tables; their low bits are not a
    # parser id, so never remap them.
    if symbol & (_COMMAND_ARG_ENUM | _COMMAND_ARG_SOFT_ENUM):
        return symbol
    arg_type = symbol & _ARG_TYPE_MASK
    new_arg_type = _ARG_TYPE_OLD_TO_NEW.get(arg_type)
    if new_arg_type is None:
        return symbol
    flags = symbol & ~_ARG_TYPE_MASK
    return flags | new_arg_type


def _remap_definition(definition: CommandDefinition) -> CommandDefinition:
    """Return a copy of a CommandDefinition with every parameter Type remapped.

    Args:
        definition: A command definition read in the v975 id space.

    Returns:
        The same definition with each overload parameter's ``symbol`` (Type)
        translated to the v1001 id space; all other fields are unchanged.
    """
    for overload in definition.overloads:
        for parameter in overload.parameters:
            parameter.symbol = _remap_arg_type(parameter.symbol)
    return definition


def rewrite_available_commands(wrapper: PacketWrapper) -> None:
    """AvailableCommands (76): remap parameter arg-type ids v975 -> v1001.

    The packet container is byte-identical between v975 and v1001; only the
    CommandParameter.Type parser ids changed. Read every command definition with
    the shared v898+ codecs (which v975 and v1001 both use), remap each
    parameter's arg-type id, and re-emit. All other arrays pass through verbatim.

    Wire layout (identical at v898 / v975 / v1001)::

        Array<string>            Enum value strings
        Array<string>            Subcommand value strings
        Array<string>            Postfixes
        Array<CommandEnum>       Enums            (indices into enum values)
        Array<CommandSubcommand> Subcommands
        Array<CommandDefinition> Commands         (parameters live here)
        Array<SoftEnum>          Soft enums
        Array<CommandConstraint> Constraints

    Args:
        wrapper: Packet wrapper for AvailableCommandsPacket.
    """
    wrapper.passthrough(ArrayType(STRING))  # Enum value strings
    wrapper.passthrough(ArrayType(STRING))  # Subcommand value strings
    wrapper.passthrough(ArrayType(STRING))  # Postfixes

    # Enums: from v898 onward (v975 and v1001 included) the enum value indices
    # are fixed-width INT_LE, modelled by COMMAND_ENUM_V898 -- NOT the v860
    # value-count-dependent width. The encoding is unchanged at v1001, so this
    # array is copied through verbatim.
    wrapper.passthrough(ArrayType(COMMAND_ENUM_V898))  # Enums
    wrapper.passthrough(ArrayType(COMMAND_SUBCOMMAND_V898))  # Subcommands

    # Commands: read each definition (v975 ids), remap parameter arg-type ids,
    # write back (v1001 ids). Container encoding is identical, so the same
    # COMMAND_DEFINITION_V898 codec reads and writes it.
    definitions = wrapper.read(ArrayType(COMMAND_DEFINITION_V898))
    remapped = [_remap_definition(d) for d in definitions]
    wrapper.write(ArrayType(COMMAND_DEFINITION_V898), remapped)

    wrapper.passthrough(ArrayType(SOFT_ENUM))  # Soft enums
    wrapper.passthrough(ArrayType(COMMAND_CONSTRAINT))  # Constraints
