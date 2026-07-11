#include "endweave/protocol/v975_to_v1001.h"

#include "endweave/protocol/attribute_layer_sync.h"

namespace endweave::v975_to_v1001 {

Protocol create_protocol()
{
    Protocol protocol{975, 1001};
    protocol.register_clientbound(AttributeLayerSyncV975::Id, translate(&upgrade));
    return protocol;
}

}  // namespace endweave::v975_to_v1001
