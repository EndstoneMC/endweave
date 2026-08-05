#include "endweave/protocols/v1001/presence.h"

#include <optional>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::PresenceConfiguration_<2168> Transformer<bp::PresenceConfiguration_<1001>, bp::PresenceConfiguration_<2168>>::
    transform(bp::PresenceConfiguration_<1001> &&from)
{
    bp::PresenceConfiguration_<2168> to;
    // ENDWEAVE: 2168 keeps only the rich presence id; experience_name and world_name are dropped.
    to.rich_presence_id = std::move(from.rich_presence_id);
    return to;
}

bp::GatheringsConfigurationJoinInfo_<2168> Transformer<
    bp::GatheringsConfigurationJoinInfo_<1001>,
    bp::GatheringsConfigurationJoinInfo_<2168>>::transform(bp::GatheringsConfigurationJoinInfo_<1001> &&from)
{
    bp::GatheringsConfigurationJoinInfo_<2168> to;
    // ENDWEAVE: 2168 made four of these optional; a 1001 packet always carries them, so all go across present.
    to.experience_id = from.experience_id;
    to.experience_name = std::move(from.experience_name);
    to.experience_world_id = from.experience_world_id;
    to.experience_world_name = std::move(from.experience_world_name);
    to.creator_id = std::move(from.creator_id);
    to.target_id = from.target_id;
    to.scenario_id = std::move(from.scenario_id);
    to.server_id = std::move(from.server_id);
    return to;
}

bp::ServerConfigurationJoinInfo_<2168> Transformer<
    bp::ServerConfigurationJoinInfo_<1001>,
    bp::ServerConfigurationJoinInfo_<2168>>::transform(bp::ServerConfigurationJoinInfo_<1001> &&from)
{
    bp::ServerConfigurationJoinInfo_<2168> to;
    to.gatherings_configuration_join_info = ew::transform(std::move(from.gatherings_configuration_join_info));
    to.client_store_entrypoint_configuration = std::move(from.client_store_entrypoint_configuration);
    to.presence_configuration = ew::transform(std::move(from.presence_configuration));
    return to;
}

bp::TransferPacket_<2168> Transformer<bp::TransferPacket_<1001>, bp::TransferPacket_<2168>>::transform(
    bp::TransferPacket_<1001> &&from)
{
    bp::TransferPacket_<2168> to;
    to.destination = std::move(from.destination);
    to.destination_port = from.destination_port;
    to.reload_world = from.reload_world;
    // ENDWEAVE: 1001 sends no gatherings configuration, and absent is what 2168 writes without one.
    to.gatherings_configuration = std::nullopt;
    return to;
}

bp::ServerPresenceInfoPacket_<2168> Transformer<
    bp::ServerPresenceInfoPacket_<1001>,
    bp::ServerPresenceInfoPacket_<2168>>::transform(bp::ServerPresenceInfoPacket_<1001> &&from)
{
    bp::ServerPresenceInfoPacket_<2168> to;
    to.presence_configuration = ew::transform(std::move(from.presence_configuration));
    return to;
}

} // namespace endweave
