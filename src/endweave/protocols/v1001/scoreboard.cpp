#include "endweave/protocols/v1001/scoreboard.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

bp::ScoreboardIdentityPacketInfo_<2168> Transformer<
    bp::ScoreboardIdentityPacketInfo_<1001>,
    bp::ScoreboardIdentityPacketInfo_<2168>>::transform(bp::ScoreboardIdentityPacketInfo_<1001> &&from)
{
    bp::ScoreboardIdentityPacketInfo_<2168> to;
    to.scoreboard_id = from.scoreboard_id;
    // ENDWEAVE: absent means removal at 2168, and this entry came off 1001's update list.
    to.player_id = from.player_id.actor_unique_id;
    return to;
}

bp::SetScorePacket_<2168> Transformer<bp::SetScorePacket_<1001>, bp::SetScorePacket_<2168>>::transform(
    bp::SetScorePacket_<1001> &&from)
{
    bp::SetScorePacket_<2168> to;
    // ENDWEAVE: the packet action that gated 1001's two lists picks the 2168 variant arm.
    if (from.type == bp::ScorePacketType::REMOVE) {
        to.score_info.reserve(from.removed_score_info.size());
        for (auto &entry : from.removed_score_info) {
            bp::RemoveScore_<2168> removed;
            removed.action = bp::ScorePacketEntryAction_<2168>::REMOVE;
            removed.scoreboard_id = entry.scoreboard_id;
            // ENDWEAVE: 1001 always wrote a name, so 2168's optional stays set.
            removed.objective_name = std::move(entry.objective_name);
            // ENDWEAVE: score_value stops here. 2168's removal carries none, and the client
            // is dropping the entry anyway.
            to.score_info.emplace_back(std::move(removed));
        }
        return to;
    }

    to.score_info.reserve(from.score_info.size());
    for (auto &entry : from.score_info) {
        // ENDWEAVE: the identity type gated which id 1001 wrote, so it names the arm exactly.
        switch (entry.identity_type) {
        case bp::IdentityDefinition::Type::PLAYER: {
            bp::ChangePlayerScore_<2168> changed;
            changed.action = bp::ScorePacketEntryAction_<2168>::CHANGE_PLAYER;
            changed.scoreboard_id = entry.scoreboard_id;
            changed.objective_name = std::move(entry.objective_name);
            changed.score_value = entry.score_value;
            changed.player_id = entry.player_id;
            to.score_info.emplace_back(std::move(changed));
            break;
        }
        case bp::IdentityDefinition::Type::ENTITY: {
            bp::ChangeEntityScore_<2168> changed;
            changed.action = bp::ScorePacketEntryAction_<2168>::CHANGE_ENTITY;
            changed.scoreboard_id = entry.scoreboard_id;
            changed.objective_name = std::move(entry.objective_name);
            changed.score_value = entry.score_value;
            changed.entity_id = entry.entity_id;
            to.score_info.emplace_back(std::move(changed));
            break;
        }
        case bp::IdentityDefinition::Type::FAKE_PLAYER: {
            bp::ChangeFakePlayerScore_<2168> changed;
            changed.action = bp::ScorePacketEntryAction_<2168>::CHANGE_FAKE_PLAYER;
            changed.scoreboard_id = entry.scoreboard_id;
            changed.objective_name = std::move(entry.objective_name);
            changed.score_value = entry.score_value;
            changed.fake_player_name = std::move(entry.fake_player_name);
            to.score_info.emplace_back(std::move(changed));
            break;
        }
        // ENDWEAVE: TODO 2168 has no arm for an identity-less entry, and inventing one would
        // put the score on the wrong holder, so the entry is dropped whole.
        case bp::IdentityDefinition::Type::INVALID:
            break;
        }
    }
    return to;
}

bp::SetScoreboardIdentityPacket_<2168> Transformer<
    bp::SetScoreboardIdentityPacket_<1001>,
    bp::SetScoreboardIdentityPacket_<2168>>::transform(bp::SetScoreboardIdentityPacket_<1001> &&from)
{
    bp::SetScoreboardIdentityPacket_<2168> to;
    to.type = from.type;
    if (from.type == bp::ScoreboardIdentityPacketType::REMOVE) {
        to.identity_info.reserve(from.removed_identity_info.size());
        for (const auto &scoreboard_id : from.removed_identity_info) {
            // ENDWEAVE: 2168 folds 1001's removal list in as entries with no player id.
            bp::ScoreboardIdentityPacketInfo_<2168> info;
            info.scoreboard_id = scoreboard_id;
            to.identity_info.push_back(info);
        }
    }
    else {
        to.identity_info = ew::transform(std::move(from.identity_info));
    }
    return to;
}

} // namespace endweave
