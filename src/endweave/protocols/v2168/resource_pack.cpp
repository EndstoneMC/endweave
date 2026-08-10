#include "endweave/protocols/v2168/resource_pack.h"

#include <cstdint>
#include <expected>
#include <limits>
#include <system_error>
#include <utility>
#include <variant>

namespace endweave {

std::expected<bp::ResourcePacksInfoPacket_<1001>, std::error_code> Transformer<
    bp::ResourcePacksInfoPacket_<2168>,
    bp::ResourcePacksInfoPacket_<1001>>::transform(bp::ResourcePacksInfoPacket_<2168> &&from)
{
    // ENDWEAVE: 1001 counts the packs in a uint16, so a longer list writes a wrapped count and
    // desynchronises the stream from there on.
    if (from.resource_packs.size() > std::numeric_limits<std::uint16_t>::max()) {
        return std::unexpected(std::make_error_code(std::errc::value_too_large));
    }
    bp::ResourcePacksInfoPacket_<1001> to;
    to.resource_pack_required = from.resource_pack_required;
    to.has_addon_packs = from.has_addon_packs;
    to.has_scripts = from.has_scripts;
    to.force_disable_vibrant_visuals = from.force_disable_vibrant_visuals;
    to.world_template_id_and_version = std::move(from.world_template_id_and_version);
    to.resource_packs = std::move(from.resource_packs);
    return to;
}

bp::ResourcePackClientResponsePacket_<1001> Transformer<
    bp::ResourcePackClientResponsePacket_<2168>,
    bp::ResourcePackClientResponsePacket_<1001>>::transform(bp::ResourcePackClientResponsePacket_<2168> &&from)
{
    using Response = bp::ResourcePackClientResponsePacket_<2168>;
    bp::ResourcePackClientResponsePacket_<1001> to;
    // ENDWEAVE: 2168 writes the response as the 0..3 tag and again name-coded in the payload; the payload is
    // what the handler reads, so 1001's 1..4 response comes from there.
    std::visit(
        [&to](const auto &response) {
            to.response = response.response_type;
        },
        from.response);
    // ENDWEAVE: 1001 reads a pack list after every response; only Downloading has one, the rest send empty.
    if (auto *downloading = std::get_if<Response::Downloading>(&from.response)) {
        to.downloading_packs = std::move(downloading->downloading_packs);
    }
    return to;
}

} // namespace endweave
