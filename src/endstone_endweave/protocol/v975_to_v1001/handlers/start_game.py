"""endweave CLIENTBOUND StartGame handler for the v975 -> v1001 delta.

Chain for a 1.26.30 (protocol v1001) client is:  v1001 <- v975 <- v944.
This handler receives a *v975-format* StartGame (already produced by the
v944_to_v975 clientbound handler, which zeroes the block-registry checksum) and
rewrites it into the *v1001 wire format* a 1.26.30 client expects.

Ground truth (cross-checked byte-for-byte, CloudburstMC/Protocol branch 3.0
StartGameSerializer_v1001 chain AND Sandertv/gophertunnel start_game.go @1.26.30):
v1001 adds exactly THREE new scalar fields relative to the v944/v975 format:

  1. serverEditorConnectionPolicy        VAR_INT -- appended in writeLevelSettings
       immediately AFTER disablingPlayerInteractions (last LEVEL_SETTINGS_V944 field).
  2. allowAnonymousBlockDropsInEditorWorlds  BOOL -- appended immediately AFTER it
       (last level-settings field; next written is levelId).
  3. isLoggingChat                        BOOL -- inserted in writeBeforeNetworkPermissions
       immediately AFTER serverAuthoritativeSound and BEFORE serverConfigurationJoinInfo.

History trap (verified, do NOT "fix"): v827 inserted tickDeathSystemsEnabled before
serverAuthSound; v898 reverted it. v1001's chain routes through v898, so that field is
ABSENT in both the v975 input and the v1001 output -- this handler correctly omits it.
"""

from endstone_endweave.codec import (
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
    """Upgrade a v975-format clientbound StartGame to v1001 wire format."""
    # Header (identical v944/v975/v1001)
    wrapper.passthrough(VAR_INT64)   # uniqueEntityId
    wrapper.passthrough(UVAR_INT64)  # runtimeEntityId
    wrapper.passthrough(VAR_INT)     # playerGameType
    wrapper.passthrough(VEC3)        # playerPosition
    wrapper.passthrough(VEC2)        # rotation

    # Level settings: pass the full v924/v944 body, then INSERT the two v1001 additions.
    wrapper.passthrough(LEVEL_SETTINGS_V944)
    wrapper.write(VAR_INT, 0)        # serverEditorConnectionPolicy (NEW v1001)
    wrapper.write(BOOL, False)       # allowAnonymousBlockDropsInEditorWorlds (NEW v1001)

    # Mid fields (Level ID ... block-registry checksum); wire-identical, checksum zeroed.
    wrapper.passthrough(STRING)      # levelId
    wrapper.passthrough(STRING)      # levelName
    wrapper.passthrough(STRING)      # premiumWorldTemplateId
    wrapper.passthrough(BOOL)        # isTrial
    wrapper.passthrough(VAR_INT)     # movement.rewindHistorySize
    wrapper.passthrough(BOOL)        # movement.serverAuthoritativeBlockBreaking
    wrapper.passthrough(INT64_LE)    # currentTick
    wrapper.passthrough(VAR_INT)     # enchantmentSeed
    block_prop_count = wrapper.passthrough(UVAR_INT)  # blockProperties
    for _ in range(block_prop_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(NAMED_COMPOUND_TAG)
    wrapper.passthrough(STRING)              # multiplayerCorrelationId
    wrapper.passthrough(BOOL)                # inventoriesServerAuthoritative
    wrapper.passthrough(STRING)              # serverEngine
    wrapper.passthrough(NAMED_COMPOUND_TAG)  # playerPropertyData
    wrapper.read(INT64_LE)
    wrapper.write(INT64_LE, 0)               # zero block-registry checksum

    # Region after the checksum: worldTemplateId(16 bytes) + 3 bools, then INSERT isLoggingChat.
    wrapper.passthrough(INT64_LE)    # worldTemplateId bytes [0:8]
    wrapper.passthrough(INT64_LE)    # worldTemplateId bytes [8:16]
    # clientSideGenerationEnabled: FORCE False for translated 1.26.30 clients. The server
    # sets it true (client-side-chunk-generation-enabled=true), which lets a native client
    # procedurally generate visual chunks beyond the authoritative radius. But a 1.26.30
    # client's worldgen cannot reproduce this 1.26.12 server world, so it renders huge bands
    # of its own empty/divergent terrain while the server keeps real collision (mobs path on
    # invisible ground). Forcing it off makes the client rely on server-streamed chunks,
    # which render correctly (the hashed block palette resolves 100% v944->v1001). Native
    # v944 clients are unaffected -- only this translated wire is overridden.
    wrapper.read(BOOL)               # clientSideGenerationEnabled (discard server's true)
    wrapper.write(BOOL, False)
    wrapper.passthrough(BOOL)        # blockNetworkIdsHashed
    wrapper.passthrough(BOOL)        # networkPermissions.serverAuthSounds
    wrapper.write(BOOL, False)       # isLoggingChat (NEW v1001)

    # serverConfigurationJoinInfo (optional, normally absent) + serverId/scenarioId/
    # worldId/ownerId are wire-identical v975->v1001 -> copy verbatim.
    wrapper.passthrough_all()
