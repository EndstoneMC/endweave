/// PacketId label formatting.

#include "endweave/protocol/packet_ids.h"

#include <iomanip>
#include <sstream>

namespace endweave {

std::string packet_label(int packet_id) {
    std::stringstream hex;
    hex << "0x" << std::uppercase << std::hex;
    if (packet_id < 16) hex << "0";
    hex << packet_id;

    // Try to find the name in the PacketId enum
    // Since C++ doesn't have enum reflection, use a switch for common ones
    const char* name = nullptr;
    switch (static_cast<PacketId>(packet_id)) {
    case PacketId::LOGIN: name = "LOGIN"; break;
    case PacketId::START_GAME: name = "START_GAME"; break;
    case PacketId::ADD_ACTOR: name = "ADD_ACTOR"; break;
    case PacketId::UPDATE_BLOCK: name = "UPDATE_BLOCK"; break;
    case PacketId::TILE_EVENT: name = "TILE_EVENT"; break;
    case PacketId::ACTOR_EVENT: name = "ACTOR_EVENT"; break;
    case PacketId::MOB_EFFECT: name = "MOB_EFFECT"; break;
    case PacketId::INVENTORY_TRANSACTION: name = "INVENTORY_TRANSACTION"; break;
    case PacketId::INTERACT: name = "INTERACT"; break;
    case PacketId::SET_ACTOR_DATA: name = "SET_ACTOR_DATA"; break;
    case PacketId::ANIMATE: name = "ANIMATE"; break;
    case PacketId::TEXT: name = "TEXT"; break;
    case PacketId::AVAILABLE_COMMANDS: name = "AVAILABLE_COMMANDS"; break;
    case PacketId::COMMAND_OUTPUT: name = "COMMAND_OUTPUT"; break;
    case PacketId::BOOK_EDIT: name = "BOOK_EDIT"; break;
    case PacketId::BIOME_DEFINITION_LIST: name = "BIOME_DEFINITION_LIST"; break;
    case PacketId::LEVEL_SOUND_EVENT: name = "LEVEL_SOUND_EVENT"; break;
    case PacketId::MAP_DATA: name = "MAP_DATA"; break;
    case PacketId::REQUEST_NETWORK_SETTINGS: name = "REQUEST_NETWORK_SETTINGS"; break;
    case PacketId::DISCONNECT: name = "DISCONNECT"; break;
    case PacketId::RESOURCE_PACK_STACK: name = "RESOURCE_PACK_STACK"; break;
    case PacketId::CAMERA_INSTRUCTION: name = "CAMERA_INSTRUCTION"; break;
    case PacketId::CAMERA_AIM_ASSIST: name = "CAMERA_AIM_ASSIST"; break;
    case PacketId::CAMERA_AIM_ASSIST_PRESETS: name = "CAMERA_AIM_ASSIST_PRESETS"; break;
    case PacketId::VOXEL_SHAPES: name = "VOXEL_SHAPES"; break;
    default: break;
    }

    if (name) {
        return std::string(name) + "(" + std::to_string(packet_id) + ") (" + hex.str() + ")";
    }
    return std::to_string(packet_id) + " (" + hex.str() + ")";
}

} // namespace endweave
