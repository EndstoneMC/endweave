"""Handler for StartGamePacket (11) -- v924 server to v944 client.

v944 changed DefaultSpawn Y from uvarint to varint (UBlockPos -> BlockPos).
v944 also restructured the server join info block near the end of the packet.
We passthrough all identical fields, convert the Y encoding,
strip the v924 server join info, and write false for v944's has_server_join_info.
"""

from ....codec import (
    BOOL,
    INT64_LE,
    LEVEL_SETTINGS_V924,
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
    """Rewrite StartGamePacket from v924 to v944 format.

    Args:
        wrapper: Packet wrapper for StartGamePacket.
    """
    wrapper.passthrough(VAR_INT64)  # Entity ID
    wrapper.passthrough(UVAR_INT64)  # Runtime ID
    wrapper.passthrough(VAR_INT)  # Game Type
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC2)  # Rotation

    wrapper.map(LEVEL_SETTINGS_V924, LEVEL_SETTINGS_V944)

    # -- Post-LevelSettings fields (identical in v924/v944) --
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
    wrapper.passthrough(INT64_LE)  # World Template ID (MSB)
    wrapper.passthrough(INT64_LE)  # World Template ID (LSB)
    wrapper.passthrough(BOOL)  # Server Enabled ClientSide Generation
    wrapper.passthrough(BOOL)  # BlockNetworkIds Are Hashes
    wrapper.passthrough(BOOL)  # NetworkPermissions

    # -- Server join info divergence --
    # v924: Does the packet contain server join information. (bool) + optional gathering data
    # v944: Does the packet contain server join information. (bool) + different structure
    # Strip v924 data and write false for v944
    has_server_join_info = wrapper.passthrough(BOOL)  # Does the packet contain server join information.
    if has_server_join_info:
        has_gathering = wrapper.read(BOOL)
        if has_gathering:
            for _ in range(7):
                wrapper.read(STRING)
        wrapper.write(BOOL, False)  # has gathering join information
        wrapper.write(BOOL, False)  # has client store entry point information
        wrapper.write(BOOL, False)  # has presence information
