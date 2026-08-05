#include "endweave/protocols/v2168/player_list.h"

#include "endweave/protocols/v1001/skin.h"
#include "endweave/protocols/v2168/skin.h"

#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

// ENDWEAVE: TODO 2168 carries the action per entry, so one packet may hold adds and
// removes at once, where 1001 has a single packet-level action and cannot. The action of
// the first entry wins and entries of the other kind are dropped. BDS builds each packet
// from one action, so the mixed form is something the 2168 shape permits rather than
// something the wire carries; refusing the downgrade needs the error channel a transform
// does not have yet.
bp::PlayerListPacket_<1001> Transformer<bp::PlayerListPacket_<2168>>::downgrade(bp::PlayerListPacket_<2168> &&from)
{
    bp::PlayerListPacket_<1001> to;
    if (from.entries.empty()) {
        return to;
    }
    to.action = std::holds_alternative<bp::PlayerListPacketPayload_<2168>::AddEntry>(from.entries.front())
                  ? bp::PlayerListPacketType::ADD
                  : bp::PlayerListPacketType::REMOVE;

    for (auto &entry : from.entries) {
        if (auto *add = std::get_if<bp::PlayerListPacketPayload_<2168>::AddEntry>(&entry)) {
            if (to.action != bp::PlayerListPacketType::ADD) {
                continue;
            }
            auto skin = ew::downgrade(std::move(add->skin));
            bp::PlayerListEntry_<1001> out;
            out.uuid = add->uuid;
            out.id = add->actor_unique_id;
            out.name = std::move(add->name);
            out.xuid = std::move(add->xuid);
            out.platform_online_id = std::move(add->platform_online_id);
            out.build_platform = add->build_platform;
            // ENDWEAVE: 2168 carries the trusted flag inside the skin; 1001 wants it in a run of one
            // bool per entry trailing the list, so it comes back out before the skin is converted.
            to.trusted_skins.push_back(skin.trusted_skin_flag == bp::TrustedSkinFlag::TRUE);
            out.skin = ew::toLegacy(skin);
            out.is_teacher = add->is_teacher;
            out.is_host = add->is_host;
            out.is_sub_client = add->is_sub_client;
            out.color = add->color;
            to.entries.push_back(std::move(out));
            continue;
        }
        if (to.action != bp::PlayerListPacketType::REMOVE) {
            continue;
        }
        to.removed_entries.push_back(std::get<bp::PlayerListPacketPayload_<2168>::RemoveEntry>(entry).uuid);
    }
    return to;
}

} // namespace endweave
