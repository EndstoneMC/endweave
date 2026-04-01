/// v860 -> v898 protocol factory.

#include "endweave/protocol/v860_to_v898/protocol.h"

#include "endweave/codec/types/enums.h"
#include "endweave/protocol/packet_ids.h"

namespace endweave::v860_to_v898 {

namespace {

int remap_sound(int v) {
    // Remap LevelSoundEvent from v860 -> v898 (shift >= v860_start by +12)
    if (v >= static_cast<int>(sound_boundaries::v860_start)) // 566
        return v + 12; // v898_start(578) - v860_start(566) = 12
    return v;
}

} // namespace

ProtocolState create_protocol() {
    auto id = [](PacketId p) { return static_cast<int>(p); };

    auto p = std::make_unique<Protocol>(SERVER_PROTOCOL, CLIENT_PROTOCOL, "v860_to_v898");

    // TODO: implement handlers
    auto stub = [](PacketWrapper& w) { w.passthrough_all(); };

    // Cancel v898 serverbound packets unknown to v860
    p->cancel_serverbound({
        id(PacketId::CLIENTBOUND_DATA_STORE),   // 330
        id(PacketId::SERVERBOUND_DATA_STORE),   // 332
    });

    // Clientbound handlers
    p->register_clientbound(id(PacketId::ACTOR_EVENT), stub);           // 27
    p->register_clientbound(id(PacketId::ANIMATE), stub);               // 44
    p->register_clientbound(id(PacketId::MOB_EFFECT), stub);            // 28
    p->register_clientbound(id(PacketId::RESOURCE_PACK_STACK), stub);   // 7
    p->register_clientbound(id(PacketId::TEXT), stub);                   // 9
    p->register_clientbound(id(PacketId::START_GAME), stub);            // 11
    p->register_clientbound(id(PacketId::LEGACY_TELEMETRY_EVENT), stub); // 65
    p->register_clientbound(id(PacketId::AVAILABLE_COMMANDS), stub);    // 76
    p->register_clientbound(id(PacketId::COMMAND_OUTPUT), stub);        // 79
    p->register_clientbound(id(PacketId::CAMERA_AIM_ASSIST_PRESETS), stub); // 320

    // Serverbound handlers
    p->register_serverbound(id(PacketId::ANIMATE), stub);         // 44
    p->register_serverbound(id(PacketId::INTERACT), stub);        // 33
    p->register_serverbound(id(PacketId::COMMAND_REQUEST), stub); // 77
    p->register_serverbound(id(PacketId::TEXT), stub);            // 9

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

} // namespace endweave::v860_to_v898
