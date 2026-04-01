#pragma once

/// Packet translation pipeline.
///
/// Routes packets through the appropriate protocol chain per connection.
///
/// See Also:
///     com.viaversion.viaversion.protocol.ProtocolPipelineImpl

#include <string>
#include <vector>

#include "endweave/codec/wrapper.h"
#include "endweave/connection.h"
#include "endweave/debug.h"
#include "endweave/exception.h"
#include "endweave/protocol/direction.h"
#include "endweave/protocol/manager.h"
#include "endweave/protocol/protocol.h"

namespace endweave {

class ProtocolPipeline {
public:
    ProtocolPipeline(ProtocolManager& manager, ConnectionManager& connections,
                     DebugHandler debug = {})
        : manager_(manager), connections_(connections), debug_(std::move(debug)) {}

    /// Handle serverbound packet. Returns true if payload was modified, false if unchanged.
    /// Sets cancelled to true if packet should be dropped.
    bool on_packet_receive(const std::string& address, int packet_id,
                           std::string_view payload, std::string& output, bool& cancelled);

    /// Handle clientbound packet. Returns true if payload was modified.
    bool on_packet_send(const std::string& address, int packet_id,
                        std::string_view payload, std::string& output, bool& cancelled);

private:
    const std::vector<Protocol*>& get_pipeline(UserConnection& connection);

    ProtocolManager& manager_;
    ConnectionManager& connections_;
    DebugHandler debug_;
};

} // namespace endweave
