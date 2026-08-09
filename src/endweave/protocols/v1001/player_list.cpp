#include "endweave/protocols/v1001/player_list.h"

#include "endweave/protocols/v1001/skin.h"

#include <cstddef>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::PlayerListPacket_<2168> Transformer<bp::PlayerListPacket_<1001>, bp::PlayerListPacket_<2168>>::transform(
    bp::PlayerListPacket_<1001> &&from)
{
    bp::PlayerListPacket_<2168> to;
    if (from.action == bp::PlayerListPacketType::ADD) {
        to.entries.reserve(from.entries.size());
        for (std::size_t i = 0; i < from.entries.size(); ++i) {
            auto &entry = from.entries[i];
            bp::PlayerListPacketPayload_<2168>::AddEntry add;
            add.action = bp::PlayerListPacketType::ADD;
            add.uuid = entry.uuid;
            add.id = entry.id;
            add.name = std::move(entry.name);
            add.xuid = std::move(entry.xuid);
            add.platform_online_id = std::move(entry.platform_online_id);
            add.build_platform = entry.build_platform;
            add.skin = ew::transform(ew::transform_to<bp::SerializedSkinRef_<1001>>(std::move(entry.skin)));
            // ENDWEAVE: 1001 keeps the trusted flag out of the skin, in a run of one bool per entry
            // trailing the list; 2168 carries it inside.
            add.skin.trusted_skin_flag = i < from.trusted_skins.size() && from.trusted_skins[i]
                                           ? bp::TrustedSkinFlag::TRUE
                                           : bp::TrustedSkinFlag::FALSE;
            add.is_teacher = entry.is_teacher;
            add.is_host = entry.is_host;
            add.is_sub_client = entry.is_sub_client;
            add.color = entry.color;
            to.entries.emplace_back(std::move(add));
        }
        return to;
    }
    to.entries.reserve(from.removed_entries.size());
    for (const auto &uuid : from.removed_entries) {
        bp::PlayerListPacketPayload_<2168>::RemoveEntry remove;
        remove.action = bp::PlayerListPacketType::REMOVE;
        remove.uuid = uuid;
        to.entries.emplace_back(std::move(remove));
    }
    return to;
}

} // namespace endweave
