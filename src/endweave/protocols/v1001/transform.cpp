#include "endweave/protocols/v1001/transform.h"

#include <variant>

namespace endweave {

bp::v2168::SerializedNetworkItemStackDescriptor Transformer<bp::v1001::SerializedNetworkItemStackDescriptor>::transform(
    const bp::v1001::SerializedNetworkItemStackDescriptor &from)
{
    bp::v2168::SerializedNetworkItemStackDescriptor to;
    to.id = from.id;
    to.stack_size = from.stack_size;
    to.aux_value = from.aux_value;
    if (from.net_id_variant.has_value()) {
        const auto &net_id = from.net_id_variant.value();
        if (const auto *request_id = std::get_if<bp::ItemStackRequestId>(&net_id)) {
            to.net_id_variant = -2 * request_id->id - 1;
        }
        else if (const auto *legacy_id = std::get_if<bp::ItemStackLegacyRequestId>(&net_id)) {
            to.net_id_variant = -2 * legacy_id->id;
        }
        else {
            to.net_id_variant = std::get<bp::ItemStackNetId>(net_id).id;
        }
    }
    to.block_runtime_id = from.block_runtime_id;
    to.user_data_buffer = from.user_data_buffer;
    return to;
}

} // namespace endweave
