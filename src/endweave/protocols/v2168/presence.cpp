#include "endweave/protocols/v2168/presence.h"

#include <optional>
#include <string>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::PresenceConfiguration_<1001> Transformer<bp::PresenceConfiguration_<2168>>::downgrade(
    bp::PresenceConfiguration_<2168> &&from)
{
    bp::PresenceConfiguration_<1001> to;
    // ENDWEAVE: 2168 dropped both names; 1001 has them optional, so absent beats an invented label.
    to.experience_name = std::nullopt;
    to.world_name = std::nullopt;
    // ENDWEAVE: 1001 requires the id 2168 made optional; empty is all that is left to write.
    to.rich_presence_id = std::move(from.rich_presence_id).value_or(std::string{});
    return to;
}

bp::GatheringsConfigurationJoinInfo_<1001> Transformer<bp::GatheringsConfigurationJoinInfo_<2168>>::downgrade(
    bp::GatheringsConfigurationJoinInfo_<2168> &&from)
{
    bp::GatheringsConfigurationJoinInfo_<1001> to;
    to.experience_id = from.experience_id;
    to.experience_name = std::move(from.experience_name);
    // ENDWEAVE: 1001 requires the four 2168 made optional; absent becomes a null UUID or an empty string,
    // which a client reads as "none" — what absent meant.
    to.experience_world_id = from.experience_world_id.value_or(bp::UUID{});
    to.experience_world_name = std::move(from.experience_world_name).value_or(std::string{});
    to.creator_id = std::move(from.creator_id);
    to.target_id = from.target_id.value_or(bp::UUID{});
    to.scenario_id = std::move(from.scenario_id).value_or(std::string{});
    to.server_id = std::move(from.server_id).value_or(std::string{});
    return to;
}

bp::ServerConfigurationJoinInfo_<1001> Transformer<bp::ServerConfigurationJoinInfo_<2168>>::downgrade(
    bp::ServerConfigurationJoinInfo_<2168> &&from)
{
    bp::ServerConfigurationJoinInfo_<1001> to;
    to.gatherings_configuration_join_info = ew::downgrade(from.gatherings_configuration_join_info);
    to.client_store_entrypoint_configuration = std::move(from.client_store_entrypoint_configuration);
    to.presence_configuration = ew::downgrade(from.presence_configuration);
    return to;
}

bp::TransferPacket_<1001> Transformer<bp::TransferPacket_<2168>>::downgrade(bp::TransferPacket_<2168> &&from)
{
    bp::TransferPacket_<1001> to;
    to.destination = std::move(from.destination);
    to.destination_port = from.destination_port;
    to.reload_world = from.reload_world;
    // ENDWEAVE: gatherings_configuration is dropped; 1001 transfers carry the destination alone, and the
    // next StartGame brings the join info anyway.
    return to;
}

bp::ServerPresenceInfoPacket_<1001> Transformer<bp::ServerPresenceInfoPacket_<2168>>::downgrade(
    bp::ServerPresenceInfoPacket_<2168> &&from)
{
    bp::ServerPresenceInfoPacket_<1001> to;
    to.presence_configuration = ew::downgrade(from.presence_configuration);
    return to;
}

} // namespace endweave
