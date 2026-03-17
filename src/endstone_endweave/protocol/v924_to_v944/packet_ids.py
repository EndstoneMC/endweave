"""Bedrock packet IDs shared between v924 and v944.

IDs are identical across both protocol versions - no remapping needed.
Only packets that Endweave touches are listed here.
Names match MinecraftPacketIds from BDS headers.
"""

from enum import IntEnum


class PacketId(IntEnum):
    LOGIN = 1
    DISCONNECT = 5
    START_GAME = 11
    UPDATE_BLOCK = 21
    TILE_EVENT = 26
    PLAYER_ACTION = 36
    SET_SPAWN_POSITION = 43
    CONTAINER_OPEN = 46
    BLOCK_ACTOR_DATA = 56
    MAP_DATA = 67
    COMMAND_BLOCK_UPDATE = 78
    PLAY_SOUND = 86
    STRUCTURE_BLOCK_UPDATE = 90
    UPDATE_BLOCK_SYNCED = 110
    LECTERN_UPDATE = 125
    STRUCTURE_TEMPLATE_DATA_EXPORT_REQUEST = 132
    ANVIL_DAMAGE = 141
    PLAYER_AUTH_INPUT = 144
    ADD_VOLUME_ENTITY = 166
    UPDATE_SUB_CHUNK_BLOCKS = 172
    REQUEST_NETWORK_SETTINGS = 193
    OPEN_SIGN = 303
    PLAYER_TOGGLE_CRAFTER_SLOT_REQUEST = 306
    MOVEMENT_EFFECT = 318
    SERVERBOUND_DATA_STORE = 332
