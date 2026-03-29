"""BDS protocol enums ported from bedrock-headers.

Each IntEnum mirrors the corresponding C++ ``enum class`` from the
Bedrock Dedicated Server headers so handler code can reference
symbolic names instead of magic literals.
"""

import enum


class AnimateAction(enum.IntEnum):
    """AnimatePacketPayload::Action."""

    NO_ACTION = 0
    SWING = 1
    WAKE_UP = 3
    CRITICAL_HIT = 4
    MAGIC_CRITICAL_HIT = 5


class ClientboundMapItemDataType(enum.IntEnum):
    """ClientboundMapItemDataPacket::Type (bitmask flags)."""

    INVALID = 0
    TEXTURE_UPDATE = 2
    DECORATION_UPDATE = 4
    CREATION = 8


class CommandOriginType(enum.IntEnum):
    """CommandOriginData::CommandOriginType."""

    PLAYER = 0
    COMMAND_BLOCK = 1
    MINECART_COMMAND_BLOCK = 2
    DEV_CONSOLE = 3
    TEST = 4
    AUTOMATION_PLAYER = 5
    CLIENT_AUTOMATION = 6
    DEDICATED_SERVER = 7
    ENTITY = 8
    VIRTUAL = 9
    GAME_ARGUMENT = 10
    ENTITY_SERVER = 11
    PRECOMPILED = 12
    GAME_DIRECTOR_ENTITY_SERVER = 13
    SCRIPTING = 14
    EXECUTE_CONTEXT = 15


class CommandOutputType(enum.IntEnum):
    """CommandOutput::CommandOutputType."""

    NONE = 0
    LAST_OUTPUT = 1
    SILENT = 2
    ALL_OUTPUT = 3
    DATA_SET = 4


class CurrentCmdVersion(enum.IntEnum):
    """CommandVersion CurrentCmdVersion."""

    INVALID = -1
    INITIAL = 1
    POST_BLOCK_FLATTENING = 43
    TEST_FOR_BLOCK_DOES_NOT_IGNORE_BLOCK_STATE = 44
    CLONE_EXTRA_BLOCK_FILTER_FIX = 45


class ComplexInventoryTransactionType(enum.IntEnum):
    """ComplexInventoryTransaction::Type."""

    NORMAL_TRANSACTION = 0
    INVENTORY_MISMATCH = 1
    ITEM_USE_TRANSACTION = 2
    ITEM_USE_ON_ENTITY_TRANSACTION = 3
    ITEM_RELEASE_TRANSACTION = 4


class DataItemType(enum.IntEnum):
    """SynchedActorData DataItemType."""

    BYTE = 0
    SHORT = 1
    INT = 2
    FLOAT = 3
    STRING = 4
    COMPOUND_TAG = 5
    POS = 6
    INT64 = 7
    VEC3 = 8
    UNKNOWN = 9


class GameRuleType(enum.IntEnum):
    """GameRules::Type."""

    INVALID = 0
    BOOL = 1
    INT = 2
    FLOAT = 3


class InventorySourceType(enum.IntEnum):
    """InventorySource InventorySourceType."""

    CONTAINER_INVENTORY = 0
    GLOBAL_INVENTORY = 1
    WORLD_INTERACTION = 2
    CREATIVE_INVENTORY = 3
    NON_IMPLEMENTED_FEATURE_TODO = 99999


class InteractAction(enum.IntEnum):
    """InteractPacket::Action."""

    INVALID = 0
    STOP_RIDING = 3
    INTERACT_UPDATE = 4
    NPC_OPEN = 5
    OPEN_INVENTORY = 6


class MapItemTrackedActorType(enum.IntEnum):
    """MapItemTrackedActor::Type."""

    ENTITY = 0
    BLOCK_ENTITY = 1
    OTHER = 2
