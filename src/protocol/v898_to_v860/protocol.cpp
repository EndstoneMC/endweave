/// v898 -> v860 protocol factory.

#include "endweave/protocol/v898_to_v860/protocol.h"

#include "endweave/codec/types/enums.h"
#include "endweave/protocol/packet_ids.h"

namespace endweave::v898_to_v860 {

namespace {

int remap_sound(int v) {
    // Remap LevelSoundEvent from v898 -> v860 (collapse v898 sounds)
    constexpr int V898_START = 578; // sound_boundaries::v898_start
    constexpr int V860_START = 566; // sound_boundaries::v860_start
    if (v >= V898_START)
        return v - 12; // v898_start(578) - v860_start(566) = 12
    if (v >= V860_START)
        return static_cast<int>(V860_START); // clamp to Undefined_v860
    return v;
}

} // namespace

ProtocolState create_protocol() {
    auto id = [](PacketId p) { return static_cast<int>(p); };

    auto p = std::make_unique<Protocol>(SERVER_PROTOCOL, CLIENT_PROTOCOL, "v898_to_v860");

    // TODO: implement handlers
    auto stub = [](PacketWrapper& w) { w.passthrough_all(); };

    // Cancel packets unknown to v860
    p->cancel_clientbound({
        id(PacketId::CLIENTBOUND_DATA_STORE), // 330
    });
    p->cancel_serverbound({
        id(PacketId::SERVERBOUND_DATA_STORE), // 332
    });

    // Clientbound handlers (reverse of v860_to_v898 serverbound)
    p->register_clientbound(id(PacketId::ANIMATE), stub);         // 44
    p->register_clientbound(id(PacketId::INTERACT), stub);        // 33
    p->register_clientbound(id(PacketId::COMMAND_REQUEST), stub); // 77
    p->register_clientbound(id(PacketId::TEXT), stub);            // 9

    // Serverbound handlers (reverse of v860_to_v898 clientbound)
    p->register_serverbound(id(PacketId::ACTOR_EVENT), stub);           // 27
    p->register_serverbound(id(PacketId::ANIMATE), stub);               // 44
    p->register_serverbound(id(PacketId::MOB_EFFECT), stub);            // 28
    p->register_serverbound(id(PacketId::RESOURCE_PACK_STACK), stub);   // 7
    p->register_serverbound(id(PacketId::TEXT), stub);                   // 9
    p->register_serverbound(id(PacketId::START_GAME), stub);            // 11
    p->register_serverbound(id(PacketId::LEGACY_TELEMETRY_EVENT), stub); // 65
    p->register_serverbound(id(PacketId::AVAILABLE_COMMANDS), stub);    // 76
    p->register_serverbound(id(PacketId::COMMAND_OUTPUT), stub);        // 79
    p->register_serverbound(id(PacketId::CAMERA_AIM_ASSIST_PRESETS), stub); // 320

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

} // namespace endweave::v898_to_v860
