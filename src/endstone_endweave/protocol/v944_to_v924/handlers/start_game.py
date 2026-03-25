"""Handler for StartGamePacket (11) -- v944 server to v924 client."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    FLOAT_LE,
    INT64_LE,
    INT_LE,
    NAMED_COMPOUND_TAG,
    SHORT_LE,
    STRING,
    UINT_LE,
    VAR_INT,
    VAR_INT64,
    UVAR_INT,
    UVAR_INT64,
    PacketWrapper,
)


def _passthrough_game_rules(wrapper: PacketWrapper) -> None:
    count = wrapper.passthrough(UVAR_INT)
    for _ in range(count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(BOOL)
        rule_type = wrapper.passthrough(UVAR_INT)
        if rule_type == 1:
            wrapper.passthrough(BOOL)
        elif rule_type == 2:
            wrapper.passthrough(VAR_INT)
        elif rule_type == 3:
            wrapper.passthrough(FLOAT_LE)
        else:
            raise ValueError(f"Unknown game rule type: {rule_type}")


def _passthrough_experiments(wrapper: PacketWrapper) -> None:
    count = wrapper.passthrough(UINT_LE)
    for _ in range(count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)


def _passthrough_block_properties(wrapper: PacketWrapper) -> None:
    count = wrapper.passthrough(UVAR_INT)
    for _ in range(count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(NAMED_COMPOUND_TAG)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Rewrite StartGamePacket from v944 to v924 format.

    Args:
        wrapper: Packet wrapper for StartGamePacket.
    """
    wrapper.passthrough(VAR_INT64)
    wrapper.passthrough(UVAR_INT64)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(SHORT_LE)
    wrapper.passthrough(STRING)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.write(UVAR_INT, wrapper.read(VAR_INT))
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    _passthrough_game_rules(wrapper)
    _passthrough_experiments(wrapper)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(BOOL)
    wrapper.passthrough(BYTE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(VAR_INT)
    _passthrough_block_properties(wrapper)
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(NAMED_COMPOUND_TAG)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)

    wrapper.passthrough(BOOL)
    has_server_join_info = wrapper.passthrough(BOOL)
    if has_server_join_info:
        wrapper.read(BOOL)
        wrapper.read(BOOL)
        wrapper.read(BOOL)
        wrapper.write(BOOL, False)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
