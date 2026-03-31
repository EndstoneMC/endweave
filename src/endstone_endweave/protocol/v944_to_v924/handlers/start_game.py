"""Handler for StartGamePacket (11) -- v944 server to v924 client.

v944 changed DefaultSpawn Y from varint to uvarint (BlockPos -> UBlockPos).
v944 also restructured the server join info block near the end of the packet.
"""

from endstone_endweave.codec import (
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
    """Rewrite StartGamePacket from v944 to v924 format.

    Args:
        wrapper: Packet wrapper for StartGamePacket.
    """
    wrapper.passthrough(VAR_INT64)  # Entity ID
    wrapper.passthrough(UVAR_INT64)  # Runtime ID
    wrapper.passthrough(VAR_INT)  # Game Type
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC2)  # Rotation

    wrapper.map(LEVEL_SETTINGS_V944, LEVEL_SETTINGS_V924)

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
    wrapper.passthrough(INT64_LE)  # Server Block Type Registry Checksum
    wrapper.passthrough(INT64_LE)  # World Template ID (MSB)
    wrapper.passthrough(INT64_LE)  # World Template ID (LSB)
    wrapper.passthrough(BOOL)  # Server Enabled ClientSide Generation
    wrapper.passthrough(BOOL)  # BlockNetworkIds Are Hashes
    wrapper.passthrough(BOOL)  # NetworkPermissions

    # -- Server join info divergence --
    # v944: Does the packet contain server join information. (bool) + v944 structure
    # v924: Does the packet contain server join information. (bool) + v924 structure
    # Strip v944 data and write v924 form
    has_server_join_info = wrapper.passthrough(BOOL)  # Does the packet contain server join information.
    if has_server_join_info:
        wrapper.read(BOOL)  # has gathering join information
        wrapper.read(BOOL)  # has client store entry point information
        wrapper.read(BOOL)  # has presence information
        wrapper.write(BOOL, False)  # has gathering (v924 form)
