#include "endweave/protocols/v1001/resource_pack.h"

#include <utility>

namespace endweave {

bp::ResourcePacksInfoPacket_<2168> Transformer<bp::ResourcePacksInfoPacket_<1001>>::upgrade(
    bp::ResourcePacksInfoPacket_<1001> &&from)
{
    bp::ResourcePacksInfoPacket_<2168> to;
    to.resource_pack_required = from.resource_pack_required;
    to.has_addon_packs = from.has_addon_packs;
    to.has_scripts = from.has_scripts;
    to.force_disable_vibrant_visuals = from.force_disable_vibrant_visuals;
    to.world_template_id_and_version = std::move(from.world_template_id_and_version);
    // ENDWEAVE: only the length prefix changed, uint16 to uvarint32; every 1001 list fits in one.
    to.resource_packs = std::move(from.resource_packs);
    return to;
}

bp::ResourcePackClientResponsePacket_<2168> Transformer<bp::ResourcePackClientResponsePacket_<1001>>::upgrade(
    bp::ResourcePackClientResponsePacket_<1001> &&from)
{
    using Response = bp::ResourcePackClientResponsePacket_<2168>;
    bp::ResourcePackClientResponsePacket_<2168> to;
    // ENDWEAVE: 1001 numbers the response 1..4 and 2168 tags the variant 0..3; selecting the case by
    // enumerator keeps that off-by-one out of the pack handshake.
    switch (from.response) {
    case bp::ResourcePackResponse::DOWNLOADING:
        // ENDWEAVE: only Downloading carries the pack list at 2168; a list on any other response is dropped.
        to.response = Response::Downloading{from.response, std::move(from.downloading_packs)};
        break;
    case bp::ResourcePackResponse::DOWNLOADING_FINISHED:
        to.response = Response::DownloadingFinished{from.response};
        break;
    case bp::ResourcePackResponse::RESOURCE_PACK_STACK_FINISHED:
        to.response = Response::ResourcePackStackFinished{from.response};
        break;
    case bp::ResourcePackResponse::CANCEL:
    default:
        // ENDWEAVE: TODO 1001 reads the response as a raw int8; a value outside 1..4 has no case and no name
        // 2168 could write, so Cancel ends the exchange rather than stalling it.
        to.response = Response::Cancel{bp::ResourcePackResponse::CANCEL};
        break;
    }
    return to;
}

} // namespace endweave
