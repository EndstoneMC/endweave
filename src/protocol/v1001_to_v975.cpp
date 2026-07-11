#include "endweave/protocol/v1001_to_v975.h"

#include "endweave/protocol/attribute_layer_sync.h"

namespace endweave::v1001_to_v975 {

Protocol create_protocol()
{
    Protocol protocol{1001, 975};
    protocol.register_clientbound(AttributeLayerSyncV1001::Id, translate(&downgrade));
    return protocol;
}

}  // namespace endweave::v1001_to_v975
