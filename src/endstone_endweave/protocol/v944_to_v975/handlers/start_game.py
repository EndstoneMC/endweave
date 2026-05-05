"""Handler for StartGamePacket (11) -- v944 server to v975 client.

The wire format is identical between v944 and v975, but the v975 client
computes a different block registry checksum than the v944 server. Without
zeroing the checksum the client silently disconnects after world load.
"""

from ....codec import (
    BOOL,
    INT64_LE,
    LEVEL_SETTINGS_V944,
    NAMED_COMPOUND_TAG,
    STRING,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    VEC2,
    VEC3,
    PacketWrapper,
)


def rewrite_start_game(wrapper: PacketWrapper) -> None:
    """Zero the block registry checksum so the v975 client skips validation.

    Args:
        wrapper: Packet wrapper for StartGamePacket.
    """
    wrapper.passthrough(VAR_INT64)  # Entity ID
    wrapper.passthrough(UVAR_INT64)  # Runtime ID
    wrapper.passthrough(VAR_INT)  # Game Type
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC2)  # Rotation

    wrapper.passthrough(LEVEL_SETTINGS_V944)

    wrapper.passthrough(STRING)  # Level ID
    wrapper.passthrough(STRING)  # Level Name
    wrapper.passthrough(STRING)  # Template Content Identity
    wrapper.passthrough(BOOL)  # Is Trial?
    wrapper.passthrough(VAR_INT)  # Movement Settings.RewindHistorySize
    wrapper.passthrough(BOOL)  # Movement Settings.ServerAuthBlockBreaking
    wrapper.passthrough(INT64_LE)  # Level Current Time
    wrapper.passthrough(VAR_INT)  # Enchantment Seed

    block_prop_count = wrapper.passthrough(UVAR_INT)  # Block Properties
    for _ in range(block_prop_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(NAMED_COMPOUND_TAG)

    wrapper.passthrough(STRING)  # Multiplayer Correlation Id
    wrapper.passthrough(BOOL)  # Enable Item Stack Net Manager
    wrapper.passthrough(STRING)  # Server version
    wrapper.passthrough(NAMED_COMPOUND_TAG)  # Player Property Data
    wrapper.read(INT64_LE)  # Server Block Type Registry Checksum
    wrapper.write(INT64_LE, 0)  # zero checksum to skip validation
    wrapper.passthrough_all()  # remaining fields are wire-identical
