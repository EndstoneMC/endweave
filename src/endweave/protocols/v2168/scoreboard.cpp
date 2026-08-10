#include "endweave/protocols/v2168/scoreboard.h"

#include <string>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

bp::ScoreboardIdentityPacketInfo_<1001> Transformer<
    bp::ScoreboardIdentityPacketInfo_<2168>,
    bp::ScoreboardIdentityPacketInfo_<1001>>::transform(bp::ScoreboardIdentityPacketInfo_<2168> &&from)
{
    bp::ScoreboardIdentityPacketInfo_<1001> to;
    to.scoreboard_id = from.scoreboard_id;
    // ENDWEAVE: an absent id is 2168's removal, which routes to 1001's other list, so the
    // null id here never reaches a client.
    to.player_id.actor_unique_id = from.player_id.value_or(0);
    return to;
}

bp::SetScorePacket_<1001> Transformer<bp::SetScorePacket_<2168>, bp::SetScorePacket_<1001>>::transform(
    bp::SetScorePacket_<2168> &&from)
{
    bp::SetScorePacket_<1001> to;
    // ENDWEAVE: 1001 has one action per packet and BDS never mixes them, so the first entry
    // names the batch and anything disagreeing is dropped below.
    to.type = !from.score_info.empty() && std::holds_alternative<bp::RemoveScore_<2168>>(from.score_info.front())
                ? bp::ScorePacketType::REMOVE
                : bp::ScorePacketType::CHANGE;

    for (auto &entry : from.score_info) {
        if (auto *removed = std::get_if<bp::RemoveScore_<2168>>(&entry)) {
            if (to.type != bp::ScorePacketType::REMOVE) {
                continue;
            }
            bp::SetScorePacket_<1001>::RemovedScorePacketInfo info;
            info.scoreboard_id = removed->scoreboard_id;
            // ENDWEAVE: empty is what the client already shows for an objective it cannot name.
            info.objective_name = std::move(removed->objective_name).value_or(std::string{});
            // ENDWEAVE: TODO 1001 requires a score on a removal and 2168 carries none, so it
            // reads zero. The client discards the entry, so it is never displayed.
            to.removed_score_info.push_back(std::move(info));
            continue;
        }

        if (to.type != bp::ScorePacketType::CHANGE) {
            continue;
        }
        // ENDWEAVE: the variant arm picks the id 2168 carries, so it names the identity type.
        bp::ScorePacketInfo info;
        if (auto *player = std::get_if<bp::ChangePlayerScore_<2168>>(&entry)) {
            info.scoreboard_id = player->scoreboard_id;
            info.objective_name = std::move(player->objective_name);
            info.score_value = player->score_value;
            info.identity_type = bp::IdentityDefinition::Type::PLAYER;
            info.player_id = player->player_id;
        }
        else if (auto *actor = std::get_if<bp::ChangeEntityScore_<2168>>(&entry)) {
            info.scoreboard_id = actor->scoreboard_id;
            info.objective_name = std::move(actor->objective_name);
            info.score_value = actor->score_value;
            info.identity_type = bp::IdentityDefinition::Type::ENTITY;
            info.entity_id = actor->entity_id;
        }
        else {
            auto &fake = std::get<bp::ChangeFakePlayerScore_<2168>>(entry);
            info.scoreboard_id = fake.scoreboard_id;
            info.objective_name = std::move(fake.objective_name);
            info.score_value = fake.score_value;
            info.identity_type = bp::IdentityDefinition::Type::FAKE_PLAYER;
            info.fake_player_name = std::move(fake.fake_player_name);
        }
        to.score_info.push_back(std::move(info));
    }
    return to;
}

bp::SetScoreboardIdentityPacket_<1001> Transformer<
    bp::SetScoreboardIdentityPacket_<2168>,
    bp::SetScoreboardIdentityPacket_<1001>>::transform(bp::SetScoreboardIdentityPacket_<2168> &&from)
{
    bp::SetScoreboardIdentityPacket_<1001> to;
    to.type = from.type;
    if (from.type == bp::ScoreboardIdentityPacketType::REMOVE) {
        to.removed_identity_info.reserve(from.identity_info.size());
        for (const auto &info : from.identity_info) {
            // ENDWEAVE: a 1001 removal is a scoreboard id alone, so the player id goes no further.
            to.removed_identity_info.push_back(info.scoreboard_id);
        }
    }
    else {
        to.identity_info = ew::transform(std::move(from.identity_info));
    }
    return to;
}

} // namespace endweave
