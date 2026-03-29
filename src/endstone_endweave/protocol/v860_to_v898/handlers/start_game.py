"""Handler for StartGamePacket -- v860 server to v898 client."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    EXPERIMENTS_V860,
    FLOAT_LE,
    GAME_RULES,
    INT64_LE,
    INT_LE,
    NAMED_COMPOUND_TAG,
    SHORT_LE,
    STRING,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    OptionalType,
    PacketWrapper,
)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Drop TickDeathSystemsEnabled from StartGame.

    Args:
        wrapper: Packet wrapper for StartGame.
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
    wrapper.passthrough(VAR_INT)
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

    wrapper.passthrough(GAME_RULES)

    wrapper.passthrough(EXPERIMENTS_V860)
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
    wrapper.passthrough(OptionalType(BOOL))
    wrapper.passthrough(BYTE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)

    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(VAR_INT)
    block_prop_count = wrapper.passthrough(UVAR_INT)
    for _ in range(block_prop_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(NAMED_COMPOUND_TAG)
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(STRING)
    wrapper.passthrough(NAMED_COMPOUND_TAG)
    wrapper.read(INT64_LE)  # Server Block Type Registry Checksum
    wrapper.write(INT64_LE, 0)  # zero checksum to skip validation
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(INT64_LE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.read(BOOL)
