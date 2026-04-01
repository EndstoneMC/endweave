/// v898 -> v924 protocol factory.

#include "endweave/protocol/v898_to_v924/protocol.h"

#include "endweave/codec/types/enums.h"
#include "endweave/protocol/packet_ids.h"

namespace endweave::v898_to_v924 {

namespace {

int remap_sound(int v) {
    // Remap LevelSoundEvent from v898 -> v924 (shift >= v898_start by +19)
    if (v >= static_cast<int>(sound_boundaries::v898_start)) // 578
        return v + 19; // v944_start is 597, but v924 undefined is also 597
    return v;
}

} // namespace

ProtocolState create_protocol() {
    auto id = [](PacketId p) { return static_cast<int>(p); };

    auto p = std::make_unique<Protocol>(SERVER_PROTOCOL, CLIENT_PROTOCOL, "v898_to_v924");

    // TODO: implement handlers
    auto stub = [](PacketWrapper& w) { w.passthrough_all(); };

    // Cancel v924 serverbound packets unknown to v898
    p->cancel_serverbound({
        id(PacketId::SERVERBOUND_DATA_STORE), // 332
    });

    // Clientbound handlers
    p->register_clientbound(id(PacketId::START_GAME), stub);                    // 11
    p->register_clientbound(id(PacketId::TEXT), stub);                           // 9
    p->register_clientbound(id(PacketId::CLIENTBOUND_DATA_STORE), stub);        // 330
    p->register_clientbound(id(PacketId::CAMERA_AIM_ASSIST_PRESETS), stub);     // 320
    p->register_clientbound(id(PacketId::GRAPHICS_PARAMETER_OVERRIDE), stub);  // 331
    p->register_clientbound(id(PacketId::CAMERA_INSTRUCTION), stub);           // 300
    p->register_clientbound(id(PacketId::BIOME_DEFINITION_LIST), stub);        // 122
    p->register_clientbound(id(PacketId::SERVER_SCRIPT_DEBUG_DRAWER), stub);   // 328

    // Serverbound handlers
    p->register_serverbound(id(PacketId::TEXT), stub);                    // 9
    p->register_serverbound(id(PacketId::BOOK_EDIT), stub);              // 97
    p->register_serverbound(id(PacketId::SERVERBOUND_DIAGNOSTICS), stub); // 315

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

} // namespace endweave::v898_to_v924
