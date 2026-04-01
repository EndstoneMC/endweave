/// v924 -> v898 protocol factory.

#include "endweave/protocol/v924_to_v898/protocol.h"

#include "endweave/codec/types/enums.h"
#include "endweave/protocol/packet_ids.h"

namespace endweave::v924_to_v898 {

namespace {

int remap_sound(int v) {
    // Remap LevelSoundEvent from v924 -> v898 (collapse v924 sounds)
    constexpr int V924_START = 597; // sound_boundaries::v944_start (v924 undefined)
    constexpr int V898_START = 578; // sound_boundaries::v898_start
    if (v >= V924_START)
        return v - 19; // V924_START(597) - V898_START(578) = 19
    if (v >= V898_START)
        return static_cast<int>(V898_START); // clamp to Undefined_v898
    return v;
}

} // namespace

ProtocolState create_protocol() {
    auto id = [](PacketId p) { return static_cast<int>(p); };

    auto p = std::make_unique<Protocol>(SERVER_PROTOCOL, CLIENT_PROTOCOL, "v924_to_v898");

    // TODO: implement handlers
    auto stub = [](PacketWrapper& w) { w.passthrough_all(); };

    // Cancel v924-specific packets unknown to v898
    p->cancel_clientbound({
        id(PacketId::CONTAINER_REGISTRY_CLEANUP),                  // 317
        id(PacketId::MOVEMENT_EFFECT),                             // 318
        id(PacketId::CLIENT_CAMERA_AIM_ASSIST),                    // 321
        id(PacketId::CLIENT_MOVEMENT_PREDICTION_SYNC),             // 322
        id(PacketId::UPDATE_CLIENT_OPTIONS),                       // 323
        id(PacketId::PLAYER_VIDEO_CAPTURE),                        // 324
        id(PacketId::PLAYER_UPDATE_ENTITY_OVERRIDES),              // 325
        id(PacketId::PLAYER_LOCATION),                             // 326
        id(PacketId::CLIENTBOUND_CONTROL_SCHEME_SET),              // 327
        id(PacketId::SERVERBOUND_PACK_SETTING_CHANGE),             // 329
        id(PacketId::CLIENTBOUND_DATA_DRIVEN_UI_SHOW_SCREEN),     // 333
        id(PacketId::CLIENTBOUND_DATA_DRIVEN_UI_CLOSE_ALL_SCREENS), // 334
        id(PacketId::CLIENTBOUND_DATA_DRIVEN_UI_RELOAD),           // 335
        id(PacketId::CLIENTBOUND_TEXTURE_SHIFT),                   // 336
        id(PacketId::VOXEL_SHAPES),                                // 337
        id(PacketId::CAMERA_SPLINE),                               // 338
        id(PacketId::CAMERA_AIM_ASSIST_ACTOR_PRIORITY),            // 339
    });

    // Clientbound handlers (reverse of v898_to_v924 serverbound)
    p->register_clientbound(id(PacketId::TEXT), stub);                    // 9
    p->register_clientbound(id(PacketId::BOOK_EDIT), stub);              // 97
    p->register_clientbound(id(PacketId::SERVERBOUND_DIAGNOSTICS), stub); // 315

    // Serverbound handlers (reverse of v898_to_v924 clientbound)
    p->register_serverbound(id(PacketId::START_GAME), stub);                    // 11
    p->register_serverbound(id(PacketId::TEXT), stub);                           // 9
    p->register_serverbound(id(PacketId::CLIENTBOUND_DATA_STORE), stub);        // 330
    p->register_serverbound(id(PacketId::CAMERA_AIM_ASSIST_PRESETS), stub);     // 320
    p->register_serverbound(id(PacketId::GRAPHICS_PARAMETER_OVERRIDE), stub);  // 331
    p->register_serverbound(id(PacketId::CAMERA_INSTRUCTION), stub);           // 300
    p->register_serverbound(id(PacketId::BIOME_DEFINITION_LIST), stub);        // 122
    p->register_serverbound(id(PacketId::SERVER_SCRIPT_DEBUG_DRAWER), stub);   // 328

    // SoundRewriter
    SoundRewriter sound(remap_sound,
        {{static_cast<int>(ActorDataIDs::HEARTBEAT_SOUND_EVENT), remap_sound}},
        {
            static_cast<int>(ActorDataIDs::AIM_ASSIST_PRIORITY_PRESET_ID),   // 136
            static_cast<int>(ActorDataIDs::AIM_ASSIST_PRIORITY_CATEGORY_ID), // 137
            static_cast<int>(ActorDataIDs::AIM_ASSIST_PRIORITY_ACTOR_ID),    // 138
        });
    sound.register_on(*p);

    return {std::move(p), std::move(sound)};
}

} // namespace endweave::v924_to_v898
