#include "endweave/protocol/v1001_to_v975.h"

#include "endweave/protocol/attribute_layer_sync.h"
#include "endweave/protocol/packet_ids.h"

namespace endweave::v1001_to_v975 {

Protocol create_protocol()
{
    Protocol protocol{1001, 975};
    protocol.register_clientbound(AttributeLayerSyncV1001::Id, translate(&downgrade));
    // Added at the 977 step, absent from 975 -- the old client has no such id, so drop it.
    protocol.cancel_clientbound({static_cast<int>(PacketId::ClientboundUpdateSoundData)});
    return protocol;
}

}  // namespace endweave::v1001_to_v975
