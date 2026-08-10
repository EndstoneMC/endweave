#include "endweave/protocols/v2168/resource_pack.h"

#include <cstdint>
#include <expected>
#include <limits>
#include <system_error>
#include <utility>
#include <variant>

namespace endweave {

void Transformer<bp::ResourcePacksInfoPacket_<2168>, bp::ResourcePacksInfoPacket_<1001>>::transform(
    Context<bp::ResourcePacksInfoPacket_<1001>> &ctx, bp::ResourcePacksInfoPacket_<2168> &&from)
{
    // ENDWEAVE: 1001 counts the packs in a uint16, so a longer list writes a wrapped count and
    // desynchronises the stream from there on.
    if (from.resource_packs.size() > std::numeric_limits<std::uint16_t>::max()) {
        ctx.cancel();
        return;
    }
    auto &to = ctx.out();
    to.resource_pack_required = from.resource_pack_required;
    to.has_addon_packs = from.has_addon_packs;
    to.has_scripts = from.has_scripts;
    to.force_disable_vibrant_visuals = from.force_disable_vibrant_visuals;
    to.world_template_id_and_version = std::move(from.world_template_id_and_version);
    to.resource_packs = std::move(from.resource_packs);
}

void Transformer<bp::ResourcePackClientResponsePacket_<2168>, bp::ResourcePackClientResponsePacket_<1001>>::transform(
    Context<bp::ResourcePackClientResponsePacket_<1001>> &ctx, bp::ResourcePackClientResponsePacket_<2168> &&from)
{
    using Response = bp::ResourcePackClientResponsePacket_<2168>;
    auto &to = ctx.out();
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
}

} // namespace endweave
